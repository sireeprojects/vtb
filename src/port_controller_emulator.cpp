#include "port_controller_emulator.h"

namespace vtb {

PortControllerEmulator::PortControllerEmulator() : PortController() {
   // start();
}

void PortControllerEmulator::create_server() {
   vtb::info() << "Started Emulator Controller";
}

void PortControllerEmulator::monitor_and_dispatch_handler() {
   launch_worker();
}

void PortControllerEmulator::epoll_worker() {
   while (is_running_) {
   }
}

}  // namespace vtb
