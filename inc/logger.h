#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <thread>
#include <sstream>
#include <atomic>

namespace vtb {

enum class LogLevel {
   ERROR   = 0,
   DEFAULT = 1,
   FULL    = 2
};

class Logger {
public:
   static Logger& get_instance();

   void init(const std::string& filename, LogLevel level);
   LogLevel get_level() const;
   void log(LogLevel msg_level, const std::string& message);

   ~Logger();

private:
   Logger();
   Logger(const Logger&) = delete;
   Logger& operator=(const Logger&) = delete;

   void flush_loop();

   LogLevel level_;
   std::ofstream file_;
   std::stringstream buffer_;
   std::mutex mutex_;
   std::thread flush_thread_;
   std::atomic<bool> running_;
   std::atomic<bool> initialized_;
};

}
