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
class VhostController {
public:
   explicit VhostController(std::string socket_path);
   virtual ~VhostController();

   VhostController(const VhostController&) = delete;
   VhostController& operator=(const VhostController&) = delete;

   // Initialise EAL + mbuf pool + inter-thread ring.
   void init(int argc, char* argv[]);

   // Register vhost-user socket, set features, begin listening.
   void start();
   void run();

   // simple queries
   int vid() const;
   bool ready() const;

   // mem pool and ring parameters
   static uint32_t MBUF_POOL_SIZE;
   static uint32_t MBUF_CACHE_SIZE;
   static uint16_t PKT_BURST_SZ;
   static uint16_t VIRTIO_RXQ;
   static uint16_t VIRTIO_TXQ;
   static uint32_t RING_SIZE;
   static uint32_t MAX_ENQUEUE_RETRIES;

protected:
   // Device lifecycle hooks.
   virtual void on_new_device(int vid);
   virtual void on_destroy_device(int vid);
   virtual void on_vring_state_changed(int vid, uint16_t queue_id, int enable);
   struct rte_mempool* mbuf_pool() const;

private:
   // Static C callbacks forwarded to the singleton instance.
   static int  cb_new_device(int vid);
   static void cb_destroy_device(int vid);
   static int cb_vring_state_changed(int vid, uint16_t queue_id, int enable);

   std::string socket_path_{};
   struct rte_mempool* mbuf_pool_{nullptr};
   struct rte_ring* ring_{nullptr};
   int vid_{-1};
   bool eal_initialised_{false};
   bool driver_registered_{false};
   std::atomic<bool> ready_{false};
   static std::atomic<VhostController*> instance_;
};

}
