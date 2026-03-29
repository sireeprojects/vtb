// #include "port_controller.h"
#include "cmdline_parser.h"
#include "config_manager.h"
// #include "port_handler.h"
#include "vhost_controller.h"
#include "logger.h"
#include "messenger.h"
#include <vector>

// A custom structure to demonstrate complex type storage
struct PortStats {
   uint64_t rx_packets;
   uint64_t tx_packets;
   double error_rate;
};

int main(int argc, char** argv) {

   // 1. Setup Logger
   vtb::Logger::get_instance().init("dpdk_app.log", vtb::LogLevel::FULL);

   //----------------------------------------------------------------

   // 2. Initialize ConfigManager and parse CLI
   auto& config = vtb::ConfigManager::get_instance();
   if (!config.init(argc, argv)) {
      vtb::error() << "Invalid command line arguments.";
      return -1;
   }

   vtb::info() << "Starting DPDK Application Environment...";

   //----------------------------------------------------------------

   // 3. Demonstrate retrieval of CLI parameters
   try {
      uint64_t pmask = config.get_arg<uint64_t>("--portmask");
      std::string mode = config.get_arg<std::string>("--mode");
      int burst = config.get_arg<int>("--burst");
      bool promisc = config.get_arg<bool>("--promisc");

      vtb::info() << "CLI: Portmask="
         << std::hex << pmask
         << " Mode=" << mode
         << " Burst=" << std::dec << burst
         << " Promisc="
         << (promisc ? "ON" : "OFF");
   } catch (...) {
      vtb::error() << "Failed to fetch one or more CLI arguments.";
   }

   //----------------------------------------------------------------

   // 4. Demonstrate "Database" functionality with all basic types
   vtb::info() << "Populating Internal Config Database...";

   // Integer / Unsigned types
   config.set_value("max_lcores", 16);
   config.set_value("socket_id", 0u);

   // Boolean
   config.set_value("hugepages_active", true);

   // String (using ""s literal for std::string)
   using namespace std::string_literals;
   config.set_value("driver_name", "net_virtio"s);

   // Floating Point
   config.set_value("timeout_ms", 500.5);

   //----------------------------------------------------------------

   // 5. Demonstrate Complex/Custom Types
   PortStats eth0_stats = { 1500000, 1200000, 0.001 };
   config.set_value("stats_port_0", eth0_stats);

   //----------------------------------------------------------------

   // 6. Retrieval Tests
   vtb::info() << "Performing Retrieval Tests...";

   // Successful retrieval with value_or
   auto lcores = config.get_value<int>("max_lcores").value_or(1);
   vtb::info() << "Retrieved lcores: " << lcores;

   // Type mismatch protection test
   // Trying to get int as string
   auto failed_cast =
      config.get_value<std::string>("max_lcores");
   if (!failed_cast) {
      vtb::details()
         << "Safety Check: Correctly blocked"
         << " invalid type cast for"
         << " 'max_lcores'.";
   }

   // Custom struct retrieval
   auto stats = config.get_value<PortStats>("stats_port_0");
   if (stats) {
      vtb::info()
         << "Port 0 Stats - RX: "
         << stats->rx_packets
         << " Error Rate: "
         << stats->error_rate;
   }

   //----------------------------------------------------------------

   // 7. Final Debug Dump (Shows everything in the log)
   config.dump_config();

   vtb::info() << "Application Setup Complete.";


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
