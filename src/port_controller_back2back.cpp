#include "port_controller_back2back.h"

namespace vtb {

PortControllerBack2Back::PortControllerBack2Back() : PortController() {
   // start();
}

void PortControllerBack2Back::create_server() {
   vtb::info() << "Started Back-to-back Controller";
}

void PortControllerBack2Back::monitor_and_dispatch_handler() {
   launch_worker();
}

void PortControllerBack2Back::epoll_worker() {
   while (is_running_) {
   }
}

}  // namespace vtb
