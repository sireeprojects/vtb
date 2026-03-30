#include "cmdline_parser.h"
#include "config_manager.h"
#include "vhost_controller.h"
#include "logger.h"
#include "messenger.h"

#include <csignal>

using namespace vtb;

static VhostController* g_backend = nullptr;

// const char* socket_path = "/tmp/vhost-user.sock";
bool stop_blocking_{false};

static void signal_handler(int) {
   vtb::info() << "Something Bad happened!";
   // rte_vhost_driver_unregister(socket_path);
   rte_vhost_driver_unregister("/tmp/vhost-user.sock");
   rte_eal_cleanup();
   stop_blocking_ = true;
}

static int worker_thread(void*) {
    while (1) {
        // This is where your future rx/tx logic will go
        rte_pause();
    }
    return 0;
}

int main(int argc, char** argv) {

   const char* socket_path = "/tmp/vhost-user.sock";

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

   rte_eal_remote_launch(worker_thread, NULL, 1);
   rte_eal_mp_wait_lcore();

   vtb::info() << "Demonstration Complete.";
   return 0;
}
