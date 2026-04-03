#include "port_controller_loopback.h"

namespace vtb {

PortControllerLoopback::PortControllerLoopback() : PortController() {
}

PortControllerLoopback::~PortControllerLoopback() {
   close(abstract_fd_); // TODO check fd before closing
}

void PortControllerLoopback::create_server() {
   abstract_sockname_ =
       vtb::ConfigManager::get_instance().get_arg<std::string>("-absn");
   std::string sock_path = std::string(1, '\0') + abstract_sockname_;

   if (sock_path.size() > 0 && sock_path[0] == '\0') {
      vtb::details() << "Verified: First byte is NULL.";
   }

   vtb::info() << "Port Controller: Started Loopback Controller with "
               << abstract_sockname_;

   abstract_fd_ = vtb::create_server_socket(sock_path);
}

void PortControllerLoopback::monitor_and_dispatch_handler() {
   listen(abstract_fd_, 5);
   epoll_fd = epoll_create1(0);
   ev.events = EPOLLIN;
   ev.data.fd = abstract_fd_;
   epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abstract_fd_, &ev);
   vtb::info() << "Port Controller: Launching epoll worker";
   launch_worker();
}

void PortControllerLoopback::epoll_worker() {
   while (is_running_) {
      int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
      for (int n = 0; n < nfds; ++n) {
         int fd = events[n].data.fd;

         if (fd == abstract_fd_) {
            int client_fd = accept(abstract_fd_, NULL, NULL);
            vtb::info()
                << "Port Controller: Connected to Vhost Controller with Fd: "
                << client_fd;
            struct epoll_event client_ev;
            client_ev.events = EPOLLIN | EPOLLRDHUP;
            client_ev.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
         } else if (events[n].events & EPOLLIN) {
            ssize_t bytes = read(fd, &pdrs_, sizeof(pdrs_));

            if (bytes <= 0) {
               vtb::info() << "Port Controller: Disconnected to Vhost "
                              "Controller with Fd: "
                           << fd;
               epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
               close(fd);
            } else if (bytes == sizeof(PortDeviceRingState)) {
               process_notification(pdrs_);
            }
         }
      }
   }
}

void PortControllerLoopback::process_notification(PortDeviceRingState pdrs) {
   vtb::info() << "Port Controller Loopback: Received:"
               << "  device_id: " << pdrs.device_id
               << "  qid: " << pdrs.qid
               << "  enable: " << pdrs.enable;

   auto it = pmap_.find(pdrs.device_id);

   if (it != pmap_.end()) { // device exists
      pmap_[pdrs.device_id][pdrs.qid] = pdrs.enable;

      if (vtb::is_even(pdrs.qid)) {
         ready_ = ((pmap_[pdrs.device_id][pdrs.qid]==1) &&
                   (pmap_[pdrs.device_id][pdrs.qid+1]==1));
         if (ready_) {
            vtb::details() << "Port Controller Loopback: Even Queues ready: " 
               << pdrs.qid << ":"<< pdrs.qid+1;
            vtb::info() << "Port Controller Loopback: Even Handler called";
         }
      } else {
         ready_ = ((pmap_[pdrs.device_id][pdrs.qid]==1) && 
                   (pmap_[pdrs.device_id][pdrs.qid-1]==1));
         if (ready_) {
            vtb::details() << "Port Controller Loopback: Odd Queues ready: "
               << pdrs.qid-1 << ":"<< pdrs.qid;
            vtb::info() << "Port Controller Loopback: Odd Handler called";
            port_handler_[pdrs.device_id]->vid = pdrs.device_id;
            port_handler_[pdrs.device_id]->start(); 
         }
      }
   } else {
      pmap_[pdrs.device_id].resize(8*2, -1); // init each element to -1
      pmap_[pdrs.device_id][pdrs.qid] = pdrs.enable;
   }
}

}  // namespace vtb
