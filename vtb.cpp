#include "cmdline_parser.h"
#include "config_manager.h"
#include "vhost_controller.h"
#include "logger.h"
#include "messenger.h"

#include <csignal>

using namespace vtb;

static VhostController* g_backend = nullptr;

const char* socket_path = "/tmp/vhost-user0";
bool stop_blocking_{false};

static void signal_handler(int) {
   vtb::info() << "Something Bad happened!";
   rte_vhost_driver_unregister(socket_path);
   rte_eal_cleanup();
   stop_blocking_ = true;
}


int main(int argc, char** argv) {

   // 1. Setup Logger
   vtb::Logger::get_instance().init("vtb_run.log", vtb::LogLevel::FULL);

   // 2. Initialize ConfigManager and parse CLI
   auto& config = vtb::ConfigManager::get_instance();
   if (!config.init(argc, argv)) {
      vtb::error() << "Invalid command line arguments.";
      return -1;
   }
   vtb::info() << "Starting DPDK Application Environment...";

   try {
      VhostController backend(socket_path);
      g_backend = &backend;

      std::signal(SIGINT,  signal_handler);
      std::signal(SIGTERM, signal_handler);

      backend.init(argc, argv);
      backend.start();
   } catch (const std::exception& e) {
      std::fprintf(stderr, "fatal: %s\n", e.what());
      return EXIT_FAILURE;
   }

   while (!stop_blocking_) {
      sleep(1);
   }

   vtb::info() << "Demonstration Complete.";
   return 0;
}
