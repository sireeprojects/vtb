#include "config_manager.h"
#include "logger.h"
#include "messenger.h"

#include <csignal>
#include <cstdio>
#include <cinttypes>
#include <stdexcept>
#include <unistd.h>

#include <rte_log.h>

#include "vhost_controller.h"

namespace vtb {

#define RTE_LOGTYPE_VHDEV RTE_LOGTYPE_USER1

// Static instance pointer (supports one backend per process)
std::atomic<VhostController*> VhostController::instance_{nullptr};

int VhostController::port_cntr_ = 0;

// Ctor
VhostController::VhostController(std::string socket_path) : socket_path_{std::move(socket_path)} {
   VhostController* expected = nullptr;

   if (!instance_.compare_exchange_strong(expected, this, std::memory_order_acq_rel)) {
      throw std::runtime_error("only one VhostController instance allowed");
   }
}

VhostController::~VhostController() {
   vtb::info() << "Cleaning up Vhost Backend...";

   if (driver_registered_) {
      rte_vhost_driver_unregister(socket_path_.c_str());
      driver_registered_ = false;
   }

   if (eal_initialised_) {
      rte_eal_cleanup();
      eal_initialised_ = false;
   }

   instance_.store(nullptr, std::memory_order_release);
}

// EAL initialisation
void VhostController::init(int argc, char* argv[]) {
   int ret = rte_eal_init(argc, argv);

   if (ret < 0)
      throw std::runtime_error("EAL init failed");

   eal_initialised_ = true;
}

// Register socket, negotiate features, start listening
void VhostController::start() {
   const char* path = socket_path_.c_str();

   if (rte_vhost_driver_register(path, 0) != 0) {
      throw std::runtime_error("vhost driver register failed: " + socket_path_);
   }
   driver_registered_ = true;

   // rte_vhost_driver_disable_features(path, 1ULL << VIRTIO_NET_F_MQ); // CHECK
   rte_vhost_driver_enable_features(path, 1ULL << VIRTIO_NET_F_MRG_RXBUF);

   static const struct rte_vhost_device_ops ops = {
      .new_device          = cb_new_device,
      .destroy_device      = cb_destroy_device,
      .vring_state_changed = cb_vring_state_changed,
      .features_changed    = nullptr,
      .new_connection      = nullptr,
      .destroy_connection  = nullptr,
      .guest_notified      = nullptr,
      .guest_notify        = nullptr
   };

   if (rte_vhost_driver_callback_register(path, &ops) != 0) {
      throw std::runtime_error("vhost callback register failed");
   }

   if (rte_vhost_driver_start(path) != 0) {
      throw std::runtime_error("vhost driver start failed");
   }

   // RTE_LOG(INFO, VHDEV, "vhost-user backend ready on %s,"
      // " waiting for guest...\n", path);
   vtb::info() << "vhost-user backend ready on " << path << " waiting for guest...";
}

// run() — launches main on a lcore TODO
void VhostController::run() {
   // launch main to the first lcore
}

//------------------------------------------------------------------
// Static C callbacks -> instance dispatch
//------------------------------------------------------------------
int VhostController::cb_new_device(int vid) {
   instance_.load(std::memory_order_acquire)->on_new_device(vid);
   return 0;
}

void VhostController::cb_destroy_device(int vid) {
   instance_.load(std::memory_order_acquire)->on_destroy_device(vid);
}

int VhostController::cb_vring_state_changed(int vid, uint16_t queue_id, int enable) {
   instance_.load(std::memory_order_acquire)->on_vring_state_changed(vid, queue_id, enable);
   return 0;
}

//------------------------------------------------------------------
// Device lifecycle hooks
//------------------------------------------------------------------
void VhostController::on_new_device(int vid) {
   RTE_LOG(INFO, VHDEV, "new device vid=%d\n", vid);

    int vring_count = rte_vhost_get_vring_num(vid);
    vtb::details() << "Total Number of queues for " << vid << " is " << vring_count 
       << " for with portnum " << VhostController::port_cntr_;

    vtb::ConfigManager::get_instance().init_vhost_device(
        VhostController::port_cntr_,
        vid,
        (vring_count/2)
        );
    vtb::ConfigManager::get_instance().set_queue_state(vid, 0, 1);
    vtb::ConfigManager::get_instance().set_queue_state(vid, 1, 1);

    VhostController::port_cntr_ ++;
}

void VhostController::on_destroy_device(int vid) {
   RTE_LOG(INFO, VHDEV, "destroy device vid=%d\n", vid);
}

void VhostController::on_vring_state_changed(int vid, uint16_t queue_id, int enable) {
   RTE_LOG(INFO, VHDEV, "vring state changed vid=%d queue=%u enable=%d\n",
           vid, queue_id, enable);

   vtb::details() << "vring state changed vid=" << vid
                                    << "queue_id=" << queue_id
                                    << "enable=" << enable;

   vtb::ConfigManager::get_instance().set_queue_state(
         vid,
         queue_id,
         enable
         );


}

}
