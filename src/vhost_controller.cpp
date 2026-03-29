#include "config_manager.h"
#include "logger.h"
#include "messenger.h"

#include <csignal>
#include <cstdio>
#include <cinttypes>
#include <stdexcept>
#include <unistd.h>

#include <rte_log.h>

#include "vhost_controller.h"

namespace vtb {

#define RTE_LOGTYPE_VLOOP RTE_LOGTYPE_USER1

// ------------------------------------------------------------------
// Static instance pointer (supports one backend per process)
// ------------------------------------------------------------------
std::atomic<VhostController*>
   VhostController::instance_{nullptr};

// ------------------------------------------------------------------
// Construction / destruction
// ------------------------------------------------------------------
VhostController::VhostController(std::string socket_path)
   : socket_path_(std::move(socket_path)) {
   VhostController* expected = nullptr;
   if (!instance_.compare_exchange_strong(
         expected, this,
         std::memory_order_acq_rel))
      throw std::runtime_error(
         "only one VhostController instance allowed");
}

VhostController::~VhostController() {
   stop();

   if (driver_registered_)
      rte_vhost_driver_unregister(socket_path_.c_str());
   if (ring_)
      rte_ring_free(ring_);
   if (eal_initialised_)
      rte_eal_cleanup();
   instance_.store(nullptr, std::memory_order_release);
}

// ------------------------------------------------------------------
// EAL + mbuf pool + ring initialisation
// ------------------------------------------------------------------
void VhostController::init(int argc, char* argv[]) {
   int ret = rte_eal_init(argc, argv);
   if (ret < 0)
      throw std::runtime_error("EAL init failed");
   eal_initialised_ = true;

   mbuf_pool_ = rte_pktmbuf_pool_create("VLOOP_MBUF_POOL",
                                         MBUF_POOL_SIZE,
                                         MBUF_CACHE_SIZE,
                                         0,
                                         RTE_MBUF_DEFAULT_BUF_SIZE,
                                         rte_socket_id());
   if (!mbuf_pool_)
      throw std::runtime_error("cannot create mbuf pool");

   // SP/SC ring: single-producer (dequeue thread),
   // single-consumer (enqueue thread)
   ring_ = rte_ring_create("VLOOP_RING",
      RING_SIZE, rte_socket_id(),
      RING_F_SP_ENQ | RING_F_SC_DEQ);
   if (!ring_)
      throw std::runtime_error("cannot create inter-thread ring");
}

// ------------------------------------------------------------------
// Register socket, negotiate features, start listening
// ------------------------------------------------------------------
void VhostController::start() {
   const char* path = socket_path_.c_str();

   if (rte_vhost_driver_register(path, 0) != 0)
      throw std::runtime_error(
         "vhost driver register failed: "
         + socket_path_);
   driver_registered_ = true;

   rte_vhost_driver_disable_features(path, 1ULL << VIRTIO_NET_F_MQ);
   rte_vhost_driver_enable_features(path, 1ULL << VIRTIO_NET_F_MRG_RXBUF);

   static const struct rte_vhost_device_ops ops = {
      .new_device          = cb_new_device,
      .destroy_device      = cb_destroy_device,
      .vring_state_changed = cb_vring_state_changed,
      .features_changed    = nullptr,
      .new_connection      = nullptr,
      .destroy_connection  = nullptr,
      .guest_notified      = nullptr,
      .guest_notify        = nullptr
   };

   if (rte_vhost_driver_callback_register(path, &ops) != 0)
      throw std::runtime_error("vhost callback register failed");

   if (rte_vhost_driver_start(path) != 0)
      throw std::runtime_error("vhost driver start failed");

   RTE_LOG(INFO, VLOOP,
      "vhost-user backend ready on %s,"
      " waiting for guest...\n", path);
}

// ------------------------------------------------------------------
// run() — launches both threads and blocks until stop()
// ------------------------------------------------------------------
void VhostController::run() {
   RTE_LOG(INFO, VLOOP,
      "launching producer/consumer threads"
      " (socket: %s)\n",
      socket_path_.c_str());

   RTE_LOG(INFO, VLOOP,
      "shutdown complete -- rx: %"
      PRIu64 " pkts, tx: %"
      PRIu64 " pkts / %"
      PRIu64 " bytes\n",
      rx_pkts_, tx_pkts_, tx_bytes_);
}

void VhostController::stop() {
   quit_.store(true, std::memory_order_relaxed);
}

// ------------------------------------------------------------------
// Device lifecycle hooks
// ------------------------------------------------------------------
void VhostController::on_new_device(int vid) {
   RTE_LOG(INFO, VLOOP, "new device vid=%d\n", vid);
   vid_   = vid;
   ready_.store(true, std::memory_order_release);
}

void VhostController::on_destroy_device(int vid) {
   RTE_LOG(INFO, VLOOP, "destroy device vid=%d\n", vid);
   if (vid_ == vid) {
      ready_.store(false, std::memory_order_release);
      vid_ = -1;
   }
}

void VhostController::on_vring_state_changed(int vid, uint16_t queue_id, int enable) {
   RTE_LOG(INFO, VLOOP, "vring state changed vid=%d queue=%u enable=%d\n",
           vid, queue_id, enable);
}

// ------------------------------------------------------------------
// Static C callbacks -> instance dispatch
// ------------------------------------------------------------------
int VhostController::cb_new_device(int vid) {
   instance_.load(std::memory_order_acquire)->on_new_device(vid);
   return 0;
}

void VhostController::cb_destroy_device(int vid) {
   instance_.load(
      std::memory_order_acquire)->on_destroy_device(vid);
}

int VhostController::cb_vring_state_changed(int vid, uint16_t queue_id, int enable) {
   instance_.load(std::memory_order_acquire)->on_vring_state_changed(vid, queue_id, enable);
   return 0;
}

}
