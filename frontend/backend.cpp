#include <iostream>
#include <atomic>
#include <csignal>
#include <string>
#include <unistd.h>
#include <stdexcept>
#include <net/if.h>

#include <rte_eal.h>
#include <rte_vhost.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_common.h>
#include <rte_cycles.h>

#define BURST_SIZE 32
#define MBUF_CACHE_SIZE 256
#define MEMPOOL_SIZE 16383

// Global synchronization
static std::atomic<int> active_vid(-1);
static std::atomic<bool> vring_ready(false);
static std::atomic<bool> force_quit(false);

static void signal_handler(int signum) { (void)signum; force_quit = true; }

// --- Callbacks ---
static int new_device(int vid) {
    char ifname[IFNAMSIZ];
    rte_vhost_get_ifname(vid, ifname, sizeof(ifname));
    std::cout << "\n[CALLBACK] Device Connected: " << ifname << " (vid: " << vid << ")" << std::endl;
    active_vid.store(vid);
    return 0;
}

static void destroy_device(int vid) {
    std::cout << "\n[CALLBACK] Device Disconnected (vid: " << vid << ")" << std::endl;
    active_vid.store(-1);
    vring_ready.store(false);
}

// CRITICAL FIX: Only set vring_ready to true when the frontend enables the queue
static int vring_state_changed(int vid, uint16_t queue_id, int enable) {
    if (queue_id == 0) { // RX Queue (from backend perspective)
        std::cout << "[CALLBACK] Vring 0 state: " << (enable ? "READY" : "NOT READY") << std::endl;
        vring_ready.store(enable == 1);
    }
    return 0;
}

static const struct rte_vhost_device_ops vhost_ops = {
    .new_device = new_device,
    .destroy_device = destroy_device,
    .vring_state_changed = vring_state_changed, // Handshake Guard
};

class RobustBackend {
public:
    RobustBackend(int argc, char** argv) {
        int args_parsed = rte_eal_init(argc, argv);
        if (args_parsed < 0) throw std::runtime_error("EAL Init Failed");

        // socket_path_ = (args_parsed < argc) ? argv[args_parsed] : "/tmp/vhost-user.sock";
        socket_path_ = "/tmp/vhost-user.sock";

        mbuf_pool_ = rte_pktmbuf_pool_create("VM_POOL", MEMPOOL_SIZE, MBUF_CACHE_SIZE, 
                                             0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        if (!mbuf_pool_) throw std::runtime_error("Mbuf Pool Failed");
    }

    ~RobustBackend() {
        rte_vhost_driver_unregister(socket_path_.c_str());
        unlink(socket_path_.c_str());
        rte_eal_cleanup();
    }

    void run() {
        unlink(socket_path_.c_str());
        if (rte_vhost_driver_register(socket_path_.c_str(), 0) < 0)
            throw std::runtime_error("Registration Failed");

        rte_vhost_driver_callback_register(socket_path_.c_str(), &vhost_ops);

        if (rte_vhost_driver_start(socket_path_.c_str()) < 0)
            throw std::runtime_error("Driver Start Failed");

        std::cout << ">>> Backend Listening on: " << socket_path_ << std::endl;
        worker_loop();
    }

private:
    struct rte_mempool* mbuf_pool_;
    std::string socket_path_;

    void worker_loop() {
        struct rte_mbuf* pkts[BURST_SIZE];
        uint64_t total_pkts = 0;
        uint64_t last_print = rte_get_timer_cycles();
        uint64_t hz = rte_get_timer_hz();

        while (!force_quit) {
            int vid = active_vid.load();

            // SENTRY: Do not dequeue if vring is not fully negotiated
            if (unlikely(vid < 0 || !vring_ready.load())) {
                rte_pause();
                continue;
            }

            uint16_t n_rx = rte_vhost_dequeue_burst(vid, 1, mbuf_pool_, pkts, BURST_SIZE);
            if (n_rx > 0) {
                uint16_t n_tx = rte_vhost_enqueue_burst(vid, 0, pkts, n_rx);
                total_pkts += n_tx;
                if (unlikely(n_tx < n_rx)) {
                    for (uint16_t i = n_tx; i < n_rx; i++) rte_pktmbuf_free(pkts[i]);
                }
            }

            // Print stats every 1 second
            uint64_t now = rte_get_timer_cycles();
            if (now - last_print > hz) {
                if (total_pkts > 0) {
                    std::cout << "\r[BACKEND] Throughput: " << total_pkts << " pkts/sec" << std::flush;
                    total_pkts = 0;
                }
                last_print = now;
            }
        }
    }
};

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    try {
        RobustBackend(argc, argv).run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
