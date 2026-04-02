#pragma once

#include <atomic>
#include <thread>

#include "config_manager.h"
#include "messenger.h"
#include "port_handler.h"

namespace vtb {

class PortController {
public:
   PortController();
   virtual ~PortController();
   PortController(const PortController&) = delete;
   PortController& operator=(const PortController&) = delete;
   void start();

protected:
   virtual void create_server() = 0;
   virtual void monitor_and_dispatch_handler() = 0;
   virtual void epoll_worker() = 0;
   void launch_worker();

   std::atomic<bool> is_running_;
   std::thread worker_;

private:
   std::unique_ptr<vtb::PortHandler> port_handler_;
};

}  // namespace vtb
