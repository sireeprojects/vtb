#pragma once

#include "port_controller.h"

namespace vtb {

// Derived controller for back2back socket communication.
class port_controller_back2back : public port_controller {
public:
   port_controller_back2back();
   virtual ~port_controller_back2back() = default;

protected:
   void create_server() override;
   void monitor_and_dispatch_handler() override;
   void epoll_worker() override;
};

}  // namespace vtb
