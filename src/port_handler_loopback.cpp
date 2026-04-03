#include "port_handler_loopback.h"

#include <rte_errno.h>

namespace vtb {
   static constexpr uint32_t MBUF_POOL_SIZE  = 8191;
   static constexpr uint32_t MBUF_CACHE_SIZE = 256;
   static constexpr uint16_t PKT_BURST_SZ    = 32;
   static constexpr uint16_t VIRTIO_RXQ      = 0;
   static constexpr uint16_t VIRTIO_TXQ      = 1;
   static constexpr uint32_t RING_SIZE       = 4096;  // must be power-of-2

PortHandlerLoopback::PortHandlerLoopback() : PortHandler() {
}

PortHandlerLoopback::~PortHandlerLoopback() {
   vtb::details() << "PortHandlerLoopback: destructor: "
                  << " vid: " << vid
                  << " rxqid: " << rxqid
                  << " txqid: " << txqid;
   if (tx_mbuf_pool_) { rte_mempool_free(tx_mbuf_pool_); tx_mbuf_pool_ = nullptr; }
   if (rx_mbuf_pool_) { rte_mempool_free(rx_mbuf_pool_); rx_mbuf_pool_ = nullptr; }
   if (tx_ring_) { rte_ring_free(tx_ring_); tx_ring_ = nullptr; }
   rx_ring_ = nullptr;

}

void PortHandlerLoopback::start() {
   // return;

   // start the appropriate port controller
   auto mode = vtb::ConfigManager::get_instance().get_arg<std::string>("--mode");
   auto nof_threads = vtb::ConfigManager::get_instance().get_arg<int>("--mode-threads");

   vtb::info() << "Starting thread"
               << " VID: " << vid
               << " RXQ: " << rxqid
               << " TXQ: " << txqid
               << " Mode: " << mode
               << " ThreadMode: " << nof_threads;
   // return;
   
   // unique names for pools and rings
   std::string tx_pool_name = "TX_MBUF_POOL_" + std::to_string(vid) + "_" + std::to_string(txqid);
   std::string rx_pool_name = "RX_MBUF_POOL_" + std::to_string(vid) + "_" + std::to_string(rxqid);
   std::string tx_ring_name = "TX_RING_"      + std::to_string(vid) + "_" + std::to_string(txqid);

   // create tx_mbuf_pool_
   vtb::info() << "PortHandlerLoopback: Creating Transmit MBUF Pool" << tx_pool_name;
   tx_mbuf_pool_ = rte_pktmbuf_pool_create(tx_pool_name.c_str(),
                   MBUF_POOL_SIZE,
                   MBUF_CACHE_SIZE,
                   0,
                   RTE_MBUF_DEFAULT_BUF_SIZE,
                   rte_socket_id());
   if (!tx_mbuf_pool_)
      throw std::runtime_error("Cannot create TX MBUF Pool: " + tx_pool_name + " rte_errno:" + std::to_string(rte_errno));
   
   // create rx_mbuf_pool_
   vtb::info() << "PortHandlerLoopback: Creating Receive MBUF Pool" << rx_pool_name;
   rx_mbuf_pool_ = rte_pktmbuf_pool_create(rx_pool_name.c_str(),
                   MBUF_POOL_SIZE,
                   MBUF_CACHE_SIZE,
                   0,
                   RTE_MBUF_DEFAULT_BUF_SIZE,
                   rte_socket_id());
   if (!rx_mbuf_pool_)
      throw std::runtime_error("Cannot create RX MBUF Pool: " + rx_pool_name + " rte_errno:" + std::to_string(rte_errno));
   
   // create tx_ring_
   /* SP/SC ring: single-producer (dequeue thread), single-consumer (enqueue thread) */
   tx_ring_ = rte_ring_create(tx_ring_name.c_str(),
                  RING_SIZE, 
                  rte_socket_id(),
                  RING_F_SP_ENQ | RING_F_SC_DEQ);
   if (!tx_ring_)
       throw std::runtime_error("Cannot create inter-thread ring: " + tx_ring_name + " rte_errno:" + std::to_string(rte_errno));

   //----------------------------------------

   if (mode=="Loopback") {
      if (nof_threads == 1) {
         vtb::info() << "Inside Local:1";
         // assign tx_ring_ to rx_ring_
         rx_ring_ = tx_ring_;
         // start tx_rx_thread_
         tx_rx_thread_ = std::thread(&PortHandlerLoopback::tx_rx_worker, this);
         tx_rx_thread_.detach();
      }
      else if (nof_threads == 2) {
         vtb::info() << "Inside Local:2";
         // assign tx_ring_ to rx_ring_
         rx_ring_ = tx_ring_;
         // start tx_thread_
         tx_thread_ = std::thread(&PortHandlerLoopback::tx_worker, this);
         tx_thread_.detach();
         // start rx_thread_
         rx_thread_ = std::thread(&PortHandlerLoopback::rx_worker, this);
         rx_thread_.detach();
      }
   }
}

// --- Worker Thread Definitions ---

void PortHandlerLoopback::tx_worker() {
   // TODO
   vtb::info() << "PortHandlerLoopback: tx_worker started";
   is_running_ = true;
   while (is_running_) {
      vtb::info() << "TX running: " << is_running_;
      sleep(1);
   }
   vtb::info() << "PortHandlerLoopback: tx_worker terminated";
}

void PortHandlerLoopback::rx_worker() {
   // TODO
   vtb::info() << "PortHandlerLoopback: rx_worker started";
   is_running_ = true;
   while (is_running_) {
      vtb::info() << "RX running: " << is_running_;
      sleep(1);
   }
   vtb::info() << "PortHandlerLoopback: rx_worker terminated";
}

void PortHandlerLoopback::tx_rx_worker() {
   vtb::info() << "PortHandlerLoopback: tx_rx_worker started";
   is_running_ = true;
   while (is_running_) {
      vtb::info() << "TX_RX running: " << is_running_;
      sleep(1);
   }
   vtb::details() << "PortHandlerLoopback: tx_rx_worker terminated";
}

// --- TX Pipeline Definitions ---

void PortHandlerLoopback::dequeue_tx_packets() {
}

void PortHandlerLoopback::extract_tx_metadata() {
}

void PortHandlerLoopback::decode_tx_metadata() {
}

void PortHandlerLoopback::act_on_tx_metadata() {
}

void PortHandlerLoopback::create_tx_port_metadata() {
}

void PortHandlerLoopback::write_tx_packets() {
}

// --- RX Pipeline Definitions ---

void PortHandlerLoopback::read_rx_packets() {
}

void PortHandlerLoopback::extract_rx_metadata() {
}

void PortHandlerLoopback::decode_rx_metadata() {
}

void PortHandlerLoopback::act_on_rx_metadata() {
}

void PortHandlerLoopback::create_rx_vm_metadata() {
}

void PortHandlerLoopback::enqueue_rx_packets() {
}

} // namespace vtb
