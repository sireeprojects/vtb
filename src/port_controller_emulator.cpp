#include "port_controller_emulator.h"

namespace vtb {

port_controller_emulator::port_controller_emulator() : port_controller() {
   // start();
}

void port_controller_emulator::create_server() {
   vtb::info() << "Started Emulator Controller";
}

void port_controller_emulator::monitor_and_dispatch_handler() {
   launch_worker();
}

void port_controller_emulator::epoll_worker() {
   while (is_running_) {
   }
}

} // namespace vtb
