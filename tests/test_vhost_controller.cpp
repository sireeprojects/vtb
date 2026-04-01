#include <termios.h>
#include <unistd.h>

#include <csignal>

#include "cmdline_parser.h"
#include "config_manager.h"
#include "logger.h"
#include "messenger.h"
#include "vhost_controller.h"

using namespace vtb;

static struct termios old_t, new_t;
bool stop_blocking_{false};

static void disable_echoctl() {
   // Get current terminal settings
   tcgetattr(STDIN_FILENO, &old_t);
   new_t = old_t;

   // Hide the ^C (Disable ECHOCTL)
   new_t.c_lflag &= ~ECHOCTL;
   tcsetattr(STDIN_FILENO, TCSANOW, &new_t);
}

static void restore_echoctl() { tcsetattr(STDIN_FILENO, TCSANOW, &old_t); }

static int worker_thread(void*) {
   while (!stop_blocking_) {
      rte_pause();
   }
   return 0;
}

static void signal_handler(int) {
   vtb::info() << "User Interruption";
   stop_blocking_ = true;
}

int main(int argc, char** argv) {
   disable_echoctl();

   // setup logger
   vtb::Logger::get_instance().init("vtb_run.log", vtb::LogLevel::DEFAULT);

   // setup configuration manager
   auto& config = vtb::ConfigManager::get_instance();
   if (!config.init(argc, argv)) {
      return -1;
   }

   // assign signal handler for graceful exit
   std::signal(SIGINT, signal_handler);
   std::signal(SIGTERM, signal_handler);

   // vhost and eal messages to just errors
   rte_log_set_level_pattern("lib.vhost.config", RTE_LOG_ERR);
   rte_log_set_level_pattern("lib.eal", RTE_LOG_ERR);

   const char* socket_path = "/tmp/vhost-user.sock";

   VhostController backend(socket_path);
   backend.init(argc, argv);
   backend.start();

   rte_eal_remote_launch(worker_thread, NULL, 2);
   rte_eal_mp_wait_lcore();

   // check vhost devices and queues for ports
   config.print_portmap();

   restore_echoctl();

   vtb::info() << "Demonstration Complete.";
   return 0;
}
