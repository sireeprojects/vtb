#pragma once

#include "port_controller.h"

namespace vtb {

// Derived controller for back2back socket communication.
class PortControllerBack2Back : public PortController {
public:
   PortControllerBack2Back();
   virtual ~PortControllerBack2Back() = default;

protected:
   void create_server() override;
   void monitor_and_dispatch_handler() override;
   void epoll_worker() override;
};

}  // namespace vtb
