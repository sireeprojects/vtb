#pragma once

#include <rte_mbuf.h>

#include <atomic>
#include <thread>
#include <stdexcept>
#include <cstdint>

namespace vtb {

// Abstract base class (interface) for port handling.
// TX and RX pipelines run on dedicated threads.
// All pipeline methods are protected and are not
// accessible to external users.

class PortHandler {
public:
   PortHandler();
   virtual ~PortHandler();

   PortHandler(const PortHandler&) = delete;
   PortHandler& operator=(const PortHandler&) = delete;

   // Starts both transmit and receive threads.
   virtual void start();

   // Signals both threads to stop.
   void stop();

protected:
   // TX pipeline — called in sequence by the transmit thread
   virtual void dequeue_tx_packets() = 0;
   virtual void extract_tx_metadata() = 0;
   virtual void decode_tx_metadata() = 0;
   virtual void act_on_tx_metadata() = 0;
   virtual void create_tx_port_metadata() = 0;
   virtual void write_tx_packets() = 0;

   // RX pipeline — called in sequence by the receive thread
   virtual void read_rx_packets() = 0;
   virtual void extract_rx_metadata() = 0;
   virtual void decode_rx_metadata() = 0;
   virtual void act_on_rx_metadata() = 0;
   virtual void create_rx_vm_metadata() = 0;
   virtual void enqueue_rx_packets() = 0;

   // Thread entry points — run the pipeline loops.
   virtual void tx_worker() = 0;
   virtual void rx_worker() = 0;
   virtual void tx_rx_worker() = 0;

   std::atomic<bool> is_running_;
   std::thread tx_thread_;
   std::thread rx_thread_;
   std::thread tx_rx_thread_;

   struct rte_mempool *tx_mbuf_pool_;
   struct rte_mempool *rx_mbuf_pool_;
   struct rte_mempool *tx_rx_mbuf_pool_;

   struct rte_ring *tx_ring_;
   struct rte_ring *rx_ring_;
   struct rte_ring *tx_rx_ring_;

   // statistics counters
   uint64_t tx_pkt_cnt_{0};
   uint64_t rx_pkt_cnt_{0};
   // TODO add if any other data needs to be captured

   // performance statistics
};

} // namespace vtb
