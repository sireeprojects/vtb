#include "port_handler_loopback.h"

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
}

void PortHandlerLoopback::start() {

   vtb::info() << "Starting thread";
   return;

   // create tx_mbuf_pool_
   vtb::info() << "PortHandlerLoopback: Creating Transmit MBUF Pool";
   tx_mbuf_pool_ = rte_pktmbuf_pool_create("TX_MBUF_POOL",
                      MBUF_POOL_SIZE,
                      MBUF_CACHE_SIZE,
                      0,
                      RTE_MBUF_DEFAULT_BUF_SIZE,
                      rte_socket_id());
   if (!tx_mbuf_pool_)
      throw std::runtime_error("cannot create tx mbuf pool");

   // create rx_mbuf_pool_
   vtb::info() << "PortHandlerLoopback: Creating Receive MBUF Pool";
   rx_mbuf_pool_ = rte_pktmbuf_pool_create("RX_MBUF_POOL",
                      MBUF_POOL_SIZE,
                      MBUF_CACHE_SIZE,
                      0,
                      RTE_MBUF_DEFAULT_BUF_SIZE,
                      rte_socket_id());
   if (!rx_mbuf_pool_)
      throw std::runtime_error("cannot create rx mbuf pool");
   
   // create tx_rx_ring_
   vtb::info() << "PortHandlerLoopback: Creating Loopback ring";
   tx_rx_ring_ = rte_ring_create("VLOOP_RING",
                    RING_SIZE,
                    rte_socket_id(),
                    RING_F_SP_ENQ | RING_F_SC_DEQ);
   if (!tx_rx_ring_)
      throw std::runtime_error("cannot create inter-thread ring");

   tx_thread_ = std::thread(&PortHandlerLoopback::tx_worker, this);
   rx_thread_ = std::thread(&PortHandlerLoopback::rx_worker, this);
   // tx_thread_.detach(); // CHECK
   // rx_thread_.detach(); // CHECK
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
