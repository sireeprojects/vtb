#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>
#include <rte_vhost.h>

#include "common.h"

namespace vtb {

#ifndef VIRTIO_NET_F_MRG_RXBUF
#define VIRTIO_NET_F_MRG_RXBUF 15
#endif

// RAII wrapper around DPDK vhost-user backend.
// Producer thread dequeues from guest TX virtqueue into an rte_ring.
// Consumer thread dequeues from the ring
// and enqueues into guest RX virtqueue.
// Both paths are lossless — they retry
// with backpressure rather than dropping.

class VhostController {
public:
   static constexpr uint32_t MBUF_POOL_SIZE = 8191;
   static constexpr uint32_t MBUF_CACHE_SIZE = 256;
   static constexpr uint16_t PKT_BURST_SZ = 32;
   static constexpr uint16_t VIRTIO_RXQ = 0;
   static constexpr uint16_t VIRTIO_TXQ = 1;
   // RING_SIZE must be power-of-2
   static constexpr uint32_t RING_SIZE = 4096;
   static constexpr uint32_t MAX_ENQUEUE_RETRIES = 1000;

   explicit VhostController(std::string socket_path);
   virtual ~VhostController();

   VhostController(const VhostController&) = delete;
   VhostController& operator=(const VhostController&) = delete;

   // Initialise EAL + mbuf pool + inter-thread ring.
   void init(int argc, char* argv[]);

   // Register vhost-user socket, set features, begin listening.
   void start();

   // Launch producer & consumer threads; blocks until stop() is called.
   void run();

   // Signal both threads to exit.
   void stop();

   int vid() const { return vid_; }
   bool ready() const { return ready_; }

protected:
   // Device lifecycle hooks.
   virtual void on_new_device(int vid);
   virtual void on_destroy_device(int vid);
   virtual void on_vring_state_changed(int vid, uint16_t queue_id, int enable);
   struct rte_mempool* mbuf_pool() const { return mbuf_pool_; }

private:
   // Static C callbacks forwarded to the singleton instance.
   static int  cb_new_device(int vid);
   static void cb_destroy_device(int vid);
   static int cb_vring_state_changed(int vid, uint16_t queue_id, int enable);

   std::string socket_path_;
   struct rte_mempool* mbuf_pool_ = nullptr;
   struct rte_ring* ring_ = nullptr;
   int vid_ = -1;

   std::atomic<bool> ready_{false};
   std::atomic<bool> quit_{false};

   bool eal_initialised_ = false;
   bool driver_registered_ = false;

   static std::atomic<VhostController*> instance_;

   // Per-thread stats (only touched by owning thread — no atomics needed).
   uint64_t rx_pkts_  = 0;
   uint64_t tx_pkts_  = 0;
   uint64_t tx_bytes_ = 0;
};

}
