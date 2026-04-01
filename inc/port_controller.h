#pragma once

#include <atomic>
#include <thread>

#include "config_manager.h"
#include "messenger.h"

namespace vtb {

class port_controller {
public:
   port_controller();
   virtual ~port_controller();
   port_controller(const port_controller&) = delete;
   port_controller& operator=(const port_controller&) = delete;
   void start();

protected:
   virtual void create_server() = 0;
   virtual void monitor_and_dispatch_handler() = 0;
   virtual void epoll_worker() = 0;
   void launch_worker();

   std::atomic<bool> is_running_;
   std::thread worker_;
};

}  // namespace vtb
