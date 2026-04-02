#include "port_handler.h"

#define BURST_SIZE 32
#define PRIV_DATA_SIZE 48
#define MBUF_CACHE_SIZE 256
#define MEMPOOL_SIZE 8191

#define RING_SIZE 2048
#define NUM_MBUFS 8191


namespace vtb {

PortHandler::PortHandler() : is_running_{false},
                             tx_mbuf_pool_{nullptr},
                             rx_mbuf_pool_{nullptr},
                             tx_rx_mbuf_pool_{nullptr},
                             tx_ring_{nullptr},
                             rx_ring_{nullptr},
                             tx_rx_ring_{nullptr}
   {
   tx_mbuf_pool_ = rte_pktmbuf_pool_create(
      "FRONTEND_POOL", MEMPOOL_SIZE, MBUF_CACHE_SIZE, 0,
      RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
      
   if (!tx_mbuf_pool_) throw std::runtime_error("Mbuf Pool Allocation Failed");

   tx_ring_ = rte_ring_create("SPSC_VHOST", RING_SIZE, rte_socket_id(),
                          RING_F_SP_ENQ | RING_F_SC_DEQ);
   if (!tx_ring_)
      rte_exit(EXIT_FAILURE, "Ring creation failed\n");
}

PortHandler::~PortHandler() {
   stop();
   if (tx_thread_.joinable())
      tx_thread_.join();
   if (rx_thread_.joinable())
      rx_thread_.join();
   if (tx_rx_thread_.joinable())
      rx_thread_.join();
}

void PortHandler::start() {
   if (!is_running_) {
      // is_running_ = true;
      // tx_thread_ = std::thread([this]{tx_thread_func();});
      // rx_thread_ = std::thread([this]{rx_thread_func();});
   }
}

void PortHandler::stop() {
   is_running_ = false;
}

} // namespace vtb
