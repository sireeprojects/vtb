#include "config_manager.h"
#include <iomanip>
#include <sstream>

namespace vtb {

ConfigManager& ConfigManager::get_instance() {
   static ConfigManager instance;
   return instance;
}

ConfigManager::ConfigManager() {
   parser_.add_argument(
      "--help", "-h",
      "Show this help menu", false, "false");
   parser_.add_argument(
      "--portmask", "-p",
      "Hex bitmask of ports", true, "0x0");
   parser_.add_argument(
      "--mode", "-m",
      "polling | interrupt", false, "polling");
   parser_.add_argument(
      "--burst", "-b",
      "Packet burst size", false, "32");
   parser_.add_argument(
      "--promisc", "-P",
      "Enable promiscuous mode", false, "false");
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::init(int argc, char** argv) {
   try {
      parser_.parse(argc, argv);
      if (parser_.get<bool>("--help")) {
         parser_.print_usage();
         return false;
      }
      return true;
   } catch (const std::exception& e) {
      error() << "Init Error: " << e.what();
      parser_.print_usage();
      return false;
   }
}

void ConfigManager::dump_config() {
   std::lock_guard<std::mutex> lock(db_mutex_);

   info() << "--- [Config DB Dump] ---";

   for (const auto& [key, val] : database_) {
      std::string v = "[Object]";

      if (val.type() == typeid(int))
         v = std::to_string(std::any_cast<int>(val));

      else if (val.type() == typeid(bool))
         v = std::any_cast<bool>(val) ? "true" : "false";

      else if (val.type() == typeid(std::string))
         v = std::any_cast<std::string>(val);

      else if (val.type() == typeid(uint64_t)) {
         std::stringstream ss;
         ss << "0x" << std::hex << std::any_cast<uint64_t>(val);
         v = ss.str();
      }

      info() << "Key: " << std::left << std::setw(20) << key << " | Value: " << v;
   }
}

void ConfigManager::init_vhost_device(int port_id, int vid, int nof_pairs) {
   std::lock_guard<std::mutex> lock(pmap_mutex_);

   PortMap& pm = pmap_[port_id];

   pm.vd.vid = vid;
   pm.vd.nof_queue_pairs = nof_pairs;
   pm.vd.ready = true;

   for (int i = 0; i < nof_pairs; i++) {
      // Typically in vhost: RX is even (0, 2..), TX is odd (1, 3..)
      pm.vd.qp[i].rxq_id = i * 2;
      pm.vd.qp[i].txq_id = (i * 2) + 1;
      pm.vd.qp[i].rxq_enabled = false;
      pm.vd.qp[i].txq_enabled = false;

      // Initialize port fds to -1 (not connected yet)
      pm.pd.qp[i].rxq_id = -1;
      pm.pd.qp[i].txq_id = -1;
   }
}

void ConfigManager::set_queue_state(int port_id, uint16_t vring_id, bool enable) {
   std::lock_guard<std::mutex> lock(pmap_mutex_);

   auto it = pmap_.find(port_id);
   if (it == pmap_.end()) return;

   VhostDevice& vd = it->second.vd;

   for (int i = 0; i < vd.nof_queue_pairs; i++) {
      if (vd.qp[i].rxq_id == vring_id) {
         vd.qp[i].rxq_enabled = enable;
         return;
      }
      if (vd.qp[i].txq_id == vring_id) {
         vd.qp[i].txq_enabled = enable;
         return;
      }
   }
}

void ConfigManager::assign_port_socket(int port_id, int qp_idx, int socket_fd) {
   std::lock_guard<std::mutex> lock(pmap_mutex_);

   if (pmap_.count(port_id) && qp_idx < vtb::MAX_QUEUE_PAIRS) {
      // We assign the same FD to both handles
      pmap_[port_id].pd.qp[qp_idx].rxq_id = socket_fd;
      pmap_[port_id].pd.qp[qp_idx].txq_id = socket_fd;

      // Mark as ready if this is the primary queue
      if (qp_idx == 0) pmap_[port_id].vd.ready = true;
   }
}

void ConfigManager::assign_control_path(int port_id, int ctl_fd) {
   std::lock_guard<std::mutex> lock(pmap_mutex_);

   if (pmap_.count(port_id)) {
      pmap_[port_id].vd.ctlq_id = (uint16_t)ctl_fd; // Logical ID
      pmap_[port_id].pd.ctlq_id = ctl_fd;            // Physical FD
   }
}

} // namespace vtb
