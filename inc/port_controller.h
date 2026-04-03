#pragma once

#include <atomic>
#include <thread>

#include "config_manager.h"
#include "messenger.h"
#include "port_handler.h"
#include "port_handler_loopback.h"
#include <array>

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

   // std::unique_ptr<vtb::PortHandler> port_handler_;
   std::array<std::unique_ptr<vtb::PortHandler>, 8> port_handler_; // TODO harcoded value
};

}  // namespace vtb
