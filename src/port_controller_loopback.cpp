#include "port_controller_loopback.h"

namespace vtb {

port_controller_loopback::port_controller_loopback() : port_controller() {
}

void port_controller_loopback::create_server() {
   abstract_sockname_ = vtb::ConfigManager::get_instance().get_arg<std::string>("-absn");
   std::string sock_path = std::string(1, '\0') + abstract_sockname_;

   if (sock_path.size() > 0 && sock_path[0] == '\0') {
    vtb::details() << "Verified: First byte is NULL.";
   }

   vtb::info() << "Started Loopback Controller with " << abstract_sockname_;
   
   abstract_fd_ = vtb::create_socket(sock_path);
   // close(abstract_fd_);
}

void port_controller_loopback::monitor_and_dispatch_handler() {
   listen(abstract_fd_, 5);
   epoll_fd = epoll_create1(0);
   ev.events = EPOLLIN;
   ev.data.fd = abstract_fd_;
   epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abstract_fd_, &ev);
   vtb::info() << "Launching epoll worker";
   launch_worker();
}

void port_controller_loopback::epoll_worker() {
   vtb::info() << "Launching epoll worker";
   while (is_running_) {
       int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
       for (int n = 0; n < nfds; ++n) {
           int fd = events[n].data.fd;

           if (fd == abstract_fd_) {
               int client_fd = accept(abstract_fd_, NULL, NULL);
               vtb::info()<< "New Client FD: " << client_fd;
               struct epoll_event client_ev;
               client_ev.events = EPOLLIN | EPOLLRDHUP;
               client_ev.data.fd = client_fd;
               epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
           } 
           else if (events[n].events & EPOLLIN) {
               char buffer[BUFFER_SIZE];
               ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
               
               if (bytes <= 0) {
                    vtb::info()<< "Client " << fd << " disconnected.";
                   epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                   close(fd);
               } else {
                   buffer[bytes] = '\0';
                   vtb::info() << "Received: " << buffer;
               }
           }
       }
   }
}

} // namespace vtb
