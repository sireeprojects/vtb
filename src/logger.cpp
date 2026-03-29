#include "logger.h"
#include <iostream>
#include <chrono>

namespace vtb {

Logger& Logger::get_instance() {
   static Logger instance;
   return instance;
}

Logger::Logger() : level_(LogLevel::DEFAULT), running_(false), initialized_(false) {
}

void Logger::init(const std::string& filename, LogLevel level) {
   if (initialized_.exchange(true)) return;

   level_ = level;
   if (!filename.empty()) {
      file_.open(filename, std::ios::out | std::ios::app);
   }

   running_ = true;
   flush_thread_ = std::thread(&Logger::flush_loop, this);
}

LogLevel Logger::get_level() const {
   return level_;
}

void Logger::log(LogLevel msg_level, const std::string& message) {
   if (msg_level <= level_ || msg_level == vtb::LogLevel::ERROR) {
      std::lock_guard<std::mutex> lock(mutex_);
      buffer_ << message << "\n";
   }
}

void Logger::flush_loop() {
   while (running_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      std::string to_write; {
         std::lock_guard<std::mutex> lock(mutex_);
         to_write = buffer_.str();
         buffer_.str("");
         buffer_.clear();
      }

      if (!to_write.empty()) {
         std::cout << to_write << std::flush;
         if (file_.is_open()) {
            file_ << to_write << std::flush;
         }
      }
   }

   // Final drain
   std::string final_content; {
      std::lock_guard<std::mutex> lock(mutex_);
      final_content = buffer_.str();
   }
   if (!final_content.empty()) {
      std::cout << final_content << std::flush;
      if (file_.is_open()) {
         file_ << final_content << std::flush;
      }
   }
}

Logger::~Logger() {
   running_ = false;
   if (flush_thread_.joinable()) {
      flush_thread_.join();
   }
}

}
