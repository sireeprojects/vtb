#include "config_manager.h"
#include "logger.h"
#include "messenger.h"

int main(int argc, char** argv) {
   // 1. Initialize Logger
   vtb::Logger::get_instance().init("vtb_test.log", vtb::LogLevel::FULL);
   vtb::info() << "VTB PortMap Demonstration Starting...";

   // 2. Initialize ConfigManager (Simulating CLI: --portmask 0x1)
   auto& config = vtb::ConfigManager::get_instance();

   // Providing dummy arguments for the parser
   const char* dummy_argv[] = {"vtb_app", "--", "--portmask", "0x1"};
   if (!config.init(4, const_cast<char**>(dummy_argv))) {
      return -1;
   }

   // 3. Populate Port Map (Simulating Device Connection)
   // Port 0, Virtual ID 10, 2 Queue Pairs
   int port_0 = 0;
   int vid_0 = 10;
   config.init_vhost_device(port_0, vid_0, 2);

   // 4. Assign Socket FDs (Simulating backend connection)
   // Assigning a dummy FD (e.g., 50) to the first queue pair
   config.assign_port_socket(port_0, 0, 50);

   // 5. Assign Control Path
   // Logical control ID 100
   config.assign_control_path(port_0, 100);

   // 6. Set Queue States (Simulating Vhost Handshake)
   // Enable RX (0) and TX (1) for the first pair
   config.set_queue_state(port_0, 0, true);
   config.set_queue_state(port_0, 1, true);

   // 7. Add a second port for comparison
   config.init_vhost_device(1, 11, 1);
   config.assign_port_socket(1, 0, 60);
   config.assign_control_path(1, 101);

   // 8. Final Output
   vtb::info() << "Displaying Configured Port Map:";
   config.print_portmap();

   vtb::info() << "Demonstration Complete.";

   return 0;
}
