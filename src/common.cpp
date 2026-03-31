#include "common.h"
#include "port_controller_loopback.h"
#include "port_controller_back2back.h"
#include "port_controller_emulator.h"

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace vtb {

std::unique_ptr<port_controller> create_controller(std::string_view mode) {
    if (mode == "Loopback") {
        return std::make_unique<port_controller_loopback>();
    }
    if (mode == "Back2Back") {
        return std::make_unique<port_controller_back2back>();
    }
    if (mode == "Emulator") {
        return std::make_unique<port_controller_emulator>();
    }
    // TODO add error message here
    return nullptr;
}

/**
 * Creates and binds/prepares a UNIX or Abstract socket.
 * @param path The filesystem path (e.g., "/tmp/sock") or 
 * abstract name (e.g., "@my_socket").
 * @return The socket file descriptor (socket id).
 */
int create_socket(const std::string& path) {
    if (path.empty()) return -1;

    // 1. Create the socket
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    size_t path_len = path.length();
    
    // Check if the maximum path limit is exceeded
    if (path_len >= sizeof(addr.sun_path)) {
        close(sock_fd);
        return -1;
    }

    // 2. Handle Abstract vs. Path-based
    // Convention: Using '@' to denote an abstract socket in the input string
    if (path[0] == '@') {
        // Abstract socket: first byte must be '\0'
        addr.sun_path[0] = '\0';
        std::memcpy(&addr.sun_path[1], path.c_str() + 1, path_len - 1);
        
        // Length must include the null byte and the name length
        // Size: offset of sun_path + 1 (null) + name_length
        socklen_t len = offsetof(struct sockaddr_un, sun_path) + path_len;
        
        if (bind(sock_fd, (struct sockaddr*)&addr, len) < 0) {
            perror("bind abstract");
            close(sock_fd);
            return -1;
        }
    } else {
        // Standard UNIX socket: Path on filesystem
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        
        // Remove existing file if it exists (standard cleanup for AF_UNIX)
        unlink(path.c_str());

        if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind unix");
            close(sock_fd);
            return -1;
        }
    }
    return sock_fd;
}

}
