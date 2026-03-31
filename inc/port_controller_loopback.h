#pragma once

#include "port_controller.h"

#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>

namespace vtb {

// Derived controller for loopback socket communication.
class port_controller_loopback : public port_controller {
public:
   port_controller_loopback();
   virtual ~port_controller_loopback() = default;

protected:
   void create_server() override;
   void monitor_and_dispatch_handler() override;
   void epoll_worker() override;

private:
   std::string abstract_sockname_{};
   int abstract_fd_{-1};
   int epoll_fd{-1};
   struct epoll_event ev, events[MAX_EVENTS];

   static constexpr int MAX_EVENTS=10;
   static constexpr int BUFFER_SIZE=1024;
};

} // namespace vtb
