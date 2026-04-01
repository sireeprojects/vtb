#pragma once

#include "port_controller.h"

namespace vtb {

// Derived controller for emulator socket communication.
class port_controller_emulator : public port_controller {
public:
   port_controller_emulator();
   virtual ~port_controller_emulator() = default;

protected:
   void create_server() override;
   void monitor_and_dispatch_handler() override;
   void epoll_worker() override;
};

}  // namespace vtb
