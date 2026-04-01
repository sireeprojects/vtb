#include "port_controller.h"

namespace vtb {

port_controller::port_controller() : is_running_(false) {}

port_controller::~port_controller() {
   vtb::info() << "Cleanup: Port Controller";
   is_running_ = false;
   if (worker_.joinable()) {
      worker_.join();
   }
}

void port_controller::start() {
   create_server();
   monitor_and_dispatch_handler();
}

void port_controller::launch_worker() {
   if (!is_running_) {
      is_running_ = true;
      worker_ = std::thread(&port_controller::epoll_worker, this);
      vtb::set_thread_name(worker_, "epollWorker");
      worker_.detach();
   }
}

}  // namespace vtb
