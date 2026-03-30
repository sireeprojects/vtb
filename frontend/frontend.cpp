#include <iostream>
#include <vector>
#include <csignal>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

// DPDK Includes
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_cycles.h>
#include <rte_version.h> // Required header
#include <rte_config.h>

#define BURST_SIZE 32
#define PRIV_DATA_SIZE 48
#define MBUF_CACHE_SIZE 256
#define MEMPOOL_SIZE 8191

static volatile bool force_quit = false;
static void signal_handler(int signum) { (void)signum; force_quit = true; }

class CustomFrontend {
public:
    CustomFrontend(int argc, char** argv) {
        rte_log_set_level_pattern("lib.vhost.config", RTE_LOG_ERR);
        rte_log_set_level_pattern("lib.eal", RTE_LOG_ERR);

        // 1. Initialize EAL
        int ret = rte_eal_init(argc, argv);
        if (ret < 0) throw std::runtime_error("EAL Init Failed");

        // 2. Create Mbuf Pool
        mbuf_pool_ = rte_pktmbuf_pool_create("FRONTEND_POOL", MEMPOOL_SIZE, 
                                             MBUF_CACHE_SIZE, 0, 
                                             RTE_MBUF_DEFAULT_BUF_SIZE, 
                                             rte_socket_id());
        if (!mbuf_pool_) throw std::runtime_error("Mbuf Pool Allocation Failed");

         std::cout << "DPDK Version: " << rte_version();

        // 3. Initialize Port 0 (the vdev)
        setup_port(0);
    }

    void setup_port(uint16_t port_id) {
        struct rte_eth_conf port_conf;
        memset(&port_conf, 0, sizeof(struct rte_eth_conf));

        // Configure with 1 RX and 1 TX queue
        if (rte_eth_dev_configure(port_id, 3, 3, &port_conf) < 0)
            throw std::runtime_error("Failed to configure port");

        // Setup RX Queue 0
        if (rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool_) < 0)
            throw std::runtime_error("Failed to setup RX queue");

        // Setup TX Queue 0
        if (rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL) < 0)
            throw std::runtime_error("Failed to setup TX queue");

        // Setup RX Queue 1
        if (rte_eth_rx_queue_setup(port_id, 1, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool_) < 0)
            throw std::runtime_error("Failed to setup RX queue");

        // Setup TX Queue 1
        if (rte_eth_tx_queue_setup(port_id, 1, 1024, rte_eth_dev_socket_id(port_id), NULL) < 0)
            throw std::runtime_error("Failed to setup TX queue");

        // Setup RX Queue 2
        if (rte_eth_rx_queue_setup(port_id, 2, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool_) < 0)
            throw std::runtime_error("Failed to setup RX queue");

        // Setup TX Queue 2
        if (rte_eth_tx_queue_setup(port_id, 2, 1024, rte_eth_dev_socket_id(port_id), NULL) < 0)
            throw std::runtime_error("Failed to setup TX queue");

        // Start Port
        if (rte_eth_dev_start(port_id) < 0)
            throw std::runtime_error("Failed to start port");

        std::cout << ">>> Port " << port_id << " initialized and started." << std::endl;
    }

    void run() {
        uint16_t port_id = 0;
        std::cout << ">>> Traffic loop active. Sending 48-byte metadata + Ethernet frames." << std::endl;

        while (!force_quit) {
            // send_burst(port_id);
            // receive_burst(port_id);
            // Throttle slightly to keep logs readable; remove for max performance
            rte_delay_us(500000); 
        }
    }

private:
    struct rte_mempool* mbuf_pool_;

    void send_burst(uint16_t port_id) {
        struct rte_mbuf* pkts[BURST_SIZE];
        for (int i = 0; i < BURST_SIZE; i++) {
            pkts[i] = rte_pktmbuf_alloc(mbuf_pool_);
            if (!pkts[i]) continue;
            
            uint8_t *ptr = rte_pktmbuf_mtod(pkts[i], uint8_t *);
            
            // Fill 48-byte metadata
            memset(ptr, 0xAA, PRIV_DATA_SIZE); 
            
            // Ethernet Header starts after metadata
            struct rte_ether_hdr *eth = (struct rte_ether_hdr *)(ptr + PRIV_DATA_SIZE);
            rte_eth_random_addr(eth->src_addr.addr_bytes);
            rte_eth_random_addr(eth->dst_addr.addr_bytes);
            eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

            // Total length: 48 (meta) + 14 (eth) + 46 (payload) = 108 bytes
            pkts[i]->data_len = PRIV_DATA_SIZE + sizeof(struct rte_ether_hdr) + 46;
            pkts[i]->pkt_len = pkts[i]->data_len;
        }

        uint16_t sent = rte_eth_tx_burst(port_id, 0, pkts, BURST_SIZE);
        if (sent > 0) {
            std::cout << "[FRONTEND] Sent " << sent << " packets." << std::endl;
        }
        
        // Free unsent packets
        if (unlikely(sent < BURST_SIZE)) {
            for (uint16_t i = sent; i < BURST_SIZE; i++) rte_pktmbuf_free(pkts[i]);
        }
    }

    void receive_burst(uint16_t port_id) {
        struct rte_mbuf* pkts[BURST_SIZE];
        uint16_t rcved = rte_eth_rx_burst(port_id, 0, pkts, BURST_SIZE);
        if (rcved > 0) {
            std::cout << "[FRONTEND] Received " << rcved << " packets back from loopback." << std::endl;
            for (uint16_t i = 0; i < rcved; i++) rte_pktmbuf_free(pkts[i]);
        }
    }
};

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    try {
        CustomFrontend app(argc, argv);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
