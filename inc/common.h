#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace vtb {

static constexpr int MAX_QUEUE_PAIRS = 8;

struct VhostQueuePair {
   uint16_t rxq_id;
   uint16_t txq_id;
   bool rxq_enabled;
   bool txq_enabled;
};

struct VhostDevice {
   int vid;
   int nof_queue_pairs;
   VhostQueuePair qp[MAX_QUEUE_PAIRS];
   bool ready;
   uint16_t ctlq_id;
};

struct PortQueuePair {
   int rxq_id;
   int txq_id;
};

struct PortDevice {
   PortQueuePair qp[MAX_QUEUE_PAIRS];
   int ctlq_id;
};

struct PortMap {
   VhostDevice vd;
   PortDevice pd;
};

class port_controller;

// The factory function declaration
std::unique_ptr<port_controller> create_controller(std::string_view mode);

int create_socket(const std::string& path);

}
