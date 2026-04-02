#include "port_controller.h"

namespace vtb {

PortController::PortController() : is_running_(false) {
   port_handler_ = 
      std::make_unique<vtb::PortHandlerLoopback>();
}

PortController::~PortController() {
   vtb::info() << "Cleanup: Port Controller";
   is_running_ = false;
   if (worker_.joinable()) {
      worker_.join();
   }
}

void PortController::start() {
   create_server();
   monitor_and_dispatch_handler();
}

void PortController::launch_worker() {
   if (!is_running_) {
      is_running_ = true;
      worker_ = std::thread(&PortController::epoll_worker, this);
      vtb::set_thread_name(worker_, "epollWorker");
      worker_.detach();
   }
}

}  // namespace vtb
