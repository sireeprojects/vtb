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

   // 2. Hide the ^C (Disable ECHOCTL)
   new_t.c_lflag &= ~ECHOCTL;
   tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

   // 2. Adjust pointers to skip DPDK args and keep only App args (like -p)
   // int app_argc = argc - eal_parsed;
   // char** app_argv = argv + eal_parsed;

   // // 3. Setup Logger
   vtb::Logger::get_instance().init("vtb_run.log", vtb::LogLevel::FULL);

   // // 4. Pass ONLY the application arguments to ConfigManager
   // auto& config = vtb::ConfigManager::get_instance();
   // if (!config.init(app_argc, app_argv)) {
   //    return -1;
   // }
   //
   std::signal(SIGINT,  signal_handler);
   std::signal(SIGTERM, signal_handler);

   try {
      VhostController backend(socket_path);
      backend.init(argc, argv);
      backend.start();
      rte_eal_remote_launch(worker_thread, NULL, 2);
      rte_eal_mp_wait_lcore();
   } catch (const std::exception& e) {
      std::fprintf(stderr, "fatal: %s\n", e.what());
      return EXIT_FAILURE;
   }

   tcsetattr(STDIN_FILENO, TCSANOW, &old_t);

   vtb::info() << "Demonstration Complete.";
   return 0;
}
