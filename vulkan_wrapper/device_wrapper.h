/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VULKAN_WRAPPER_DEVICE_WRAPPER_H_
#define VULKAN_WRAPPER_DEVICE_WRAPPER_H_

#include <cstring>
#include <memory>

#include "support/containers/unique_ptr.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/function_table.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

namespace vulkan {

// VkDevice wraps a native vulkan VkDevice handle. It provides
// lazily initialized function pointers for all of its
// methods. It will automatically call VkDestroyDevice when it
// goes out of scope.
class VkDevice {
 public:
  VkDevice(VkDevice&& other) = default;
  // This does not retain a reference to the VkInstance, or the
  // VkAllocationCallbacks object, it does take ownership of the device.
  // If properties is not nullptr, then the device_id, vendor_id and
  // driver_version will be copied out of it.
  VkDevice(containers::Allocator* container_allocator, ::VkDevice device,
           VkAllocationCallbacks* allocator, VkInstance* instance,
           VkPhysicalDeviceProperties* properties = nullptr,
           ::VkPhysicalDevice physical_device = VK_NULL_HANDLE,
           uint32_t num_devices = 1)
      : device_(device),
        physical_device_(physical_device),
        has_allocator_(allocator != nullptr),
        log_(instance->GetLogger()),
        device_id_(0),
        vendor_id_(0),
        driver_version_(0),
        physical_device_memory_properties_({0}),
        num_devices_(num_devices) {
    if (has_allocator_) {
      allocator_ = *allocator;
    } else {
      memset(&allocator_, 0, sizeof(allocator_));
    }
    vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        instance->get_wrapper()->getProcAddr(*instance, "vkGetDeviceProcAddr"));
    LOG_ASSERT(!=, log_, vkGetDeviceProcAddr,
               static_cast<PFN_vkGetDeviceProcAddr>(nullptr));
    if (properties) {
      device_id_ = properties->deviceID;
      vendor_id_ = properties->vendorID;
      driver_version_ = properties->driverVersion;
    }
    // Initialize the lazily resolved device functions.
    functions_ = containers::make_unique<DeviceFunctions>(
        container_allocator, device_, vkGetDeviceProcAddr, log_);
    if (physical_device) {
      (*instance)->vkGetPhysicalDeviceMemoryProperties(
          physical_device, &physical_device_memory_properties_);
    }
  }

  ~VkDevice() {
    // functions_ will be nullptr if this has been moved
    if (device_ && functions_) {
      functions_->vkDestroyDevice(device_,
                                  has_allocator_ ? &allocator_ : nullptr);
    }
  }

  // To conform to the VkSubObjects::Traits interface
  PFN_vkGetDeviceProcAddr getProcAddrFunction() { return vkGetDeviceProcAddr; }

  uint32_t device_id() const { return device_id_; }
  uint32_t vendor_id() const { return vendor_id_; }
  uint32_t driver_version() const { return driver_version_; }
  uint32_t num_devices() const { return num_devices_; }

  bool is_valid() { return device_ != VK_NULL_HANDLE; }

  logging::Logger* GetLogger() { return log_; }

  DeviceFunctions* functions() { return functions_.get(); }
  ::VkPhysicalDevice physical_device() const { return physical_device_; }

  const VkPhysicalDeviceMemoryProperties& physical_device_memory_properties()
      const {
    return physical_device_memory_properties_;
  }

 private:
  ::VkDevice device_;
  ::VkPhysicalDevice physical_device_;
  bool has_allocator_;
  // Intentionally keep a copy of the callbacks, they are just a bunch of
  // pointers, but it means we don't force our user to keep the allocator struct
  // around forever.
  VkAllocationCallbacks allocator_;
  logging::Logger* log_;
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
  // Lazily resolved Vulkan device functions.
  containers::unique_ptr<DeviceFunctions> functions_;

  uint32_t device_id_;
  uint32_t vendor_id_;
  uint32_t driver_version_;
  uint32_t num_devices_;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties_;

 public:
  PFN_vkVoidFunction getProcAddr(::VkDevice device, const char* function) {
    return vkGetDeviceProcAddr(device, function);
  }
  ::VkDevice get_device() const { return device_; }
  operator ::VkDevice() const { return device_; }

  // Override operators to access the lazily resolved functions stored in
  // functions_;
  DeviceFunctions* operator->() { return functions_.get(); }
  DeviceFunctions& operator*() { return *functions_.get(); }
};

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_DEVICE_WRAPPER_H_
