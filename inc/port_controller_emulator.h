#pragma once

#include "port_controller.h"

namespace vtb {

// Derived controller for emulator socket communication.
class PortControllerEmulator : public PortController {
public:
   PortControllerEmulator();
   virtual ~PortControllerEmulator() = default;

protected:
   void create_server() override;
   void monitor_and_dispatch_handler() override;
   void epoll_worker() override;
};

}  // namespace vtb
