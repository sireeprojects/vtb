#include "port_controller_back2back.h"

namespace vtb {

port_controller_back2back::port_controller_back2back() : port_controller() {
   // start();
}

void port_controller_back2back::create_server() {
   vtb::info() << "Started Back-to-back Controller";
}

void port_controller_back2back::monitor_and_dispatch_handler() {
   launch_worker();
}

void port_controller_back2back::epoll_worker() {
   while (is_running_) {
   }
}

}  // namespace vtb
