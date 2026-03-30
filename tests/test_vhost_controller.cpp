#include "cmdline_parser.h"
#include "config_manager.h"
#include "vhost_controller.h"
#include "logger.h"
#include "messenger.h"

#include <csignal>

#include <termios.h>
#include <unistd.h>

using namespace vtb;

bool stop_blocking_{false};

static void signal_handler(int) {
   vtb::info() << "User Interruption";
   stop_blocking_ = true;
}

static int worker_thread(void*) {
    while (!stop_blocking_) {
        rte_pause();
    }
    return 0;
}

int main(int argc, char** argv) {
   const char* socket_path = "/tmp/vhost-user.sock";

   // 1. Get current terminal settings
   struct termios old_t, new_t;
   tcgetattr(STDIN_FILENO, &old_t);
   new_t = old_t;
   // Hide the ^C (Disable ECHOCTL)
   new_t.c_lflag &= ~ECHOCTL;
   tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

   // Setup Logger
   vtb::Logger::get_instance().init("vtb_run.log", vtb::LogLevel::DEFAULT);

   auto& config = vtb::ConfigManager::get_instance();
   if (!config.init(argc, argv)) {
      return -1;
   }
   
   std::signal(SIGINT,  signal_handler);
   std::signal(SIGTERM, signal_handler);

   rte_log_set_level_pattern("lib.vhost.config", RTE_LOG_ERR);
   rte_log_set_level_pattern("lib.eal", RTE_LOG_ERR);

   try {
      VhostController backend(socket_path);
      backend.init(argc, argv);

      // int vhost_logtype = rte_log_register("lib.vhost.config");
      // rte_log_set_level(vhost_logtype, RTE_LOG_ERR);

      backend.start();
      rte_eal_remote_launch(worker_thread, NULL, 2);
      rte_eal_mp_wait_lcore();

      config.print_portmap();

   } catch (const std::exception& e) {
      std::fprintf(stderr, "fatal: %s\n", e.what());
      return EXIT_FAILURE;
   }

   tcsetattr(STDIN_FILENO, TCSANOW, &old_t);

   vtb::info() << "Demonstration Complete.";
   return 0;
}
