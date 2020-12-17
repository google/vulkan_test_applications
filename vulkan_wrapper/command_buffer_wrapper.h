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

#ifndef VULKAN_WRAPPER_COMMAND_BUFFER_WRAPPER_H_
#define VULKAN_WRAPPER_COMMAND_BUFFER_WRAPPER_H_

#include <memory>

#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/lazy_function.h"
#include "vulkan_wrapper/sub_objects.h"

namespace vulkan {

// VkCommandBuffer takes the ownership and wraps a native VkCommandBuffer
// object. It provides lazily initialized function pointers for all of its
// methods. It will automatically call VkFreeCommandBuffers when it goes out of
// scope.
class VkCommandBuffer {
 public:
  VkCommandBuffer(VkCommandBuffer&& other)
      : command_buffer_(other.command_buffer_),
        pool_(other.pool_),
        device_(other.device_),
        log_(other.log_),
        destruction_function_(other.destruction_function_),
        functions_(other.functions_) {
    other.command_buffer_ = static_cast<::VkCommandBuffer>(VK_NULL_HANDLE);
  }

  VkCommandBuffer(::VkCommandBuffer command_buffer, VkCommandPool* pool,
                  VkDevice* device)
      : command_buffer_(command_buffer),
        pool_(*pool),
        device_(*device),
        log_(device->GetLogger()),
        destruction_function_(&(*device)->vkFreeCommandBuffers),
        functions_((*device)->command_buffer_functions()) {
    for (size_t i = 0; i < device->num_devices(); ++i) {
      default_mask_ |= 1 << i;
    }
  }

  ~VkCommandBuffer() {
    if (command_buffer_ != VK_NULL_HANDLE) {
      (*destruction_function_)(device_, pool_, 1, &command_buffer_);
    }
  }

  logging::Logger* GetLogger() { return log_; }

  void set_device_mask(uint32_t device_mask) {
    device_mask_ = device_mask;
    functions_->vkCmdSetDeviceMask(command_buffer_, device_mask);
  }
  uint32_t get_device_mask() { return device_mask_; }

  void begin_command_buffer(const VkCommandBufferBeginInfo* begin_info) {
    device_mask_ = default_mask_;
    const VkDeviceGroupCommandBufferBeginInfo* dgcbbi =
        reinterpret_cast<const VkDeviceGroupCommandBufferBeginInfo*>(
            begin_info->pNext);
    if (dgcbbi &&
        dgcbbi->sType ==
            VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO) {
      device_mask_ = dgcbbi->deviceMask;
    }
    functions_->vkBeginCommandBuffer(command_buffer_, begin_info);
  }

 private:
  ::VkCommandBuffer command_buffer_;
  ::VkCommandPool pool_;
  ::VkDevice device_;
  logging::Logger* log_;
  LazyFunction<PFN_vkFreeCommandBuffers, ::VkDevice, DeviceFunctions>*
      destruction_function_;
  CommandBufferFunctions* functions_;
  uint32_t device_mask_ = 0;
  uint32_t default_mask_ = 0;

 public:
  const ::VkCommandBuffer& get_command_buffer() const {
    return command_buffer_;
  }
  operator ::VkCommandBuffer() const { return command_buffer_; }
  CommandBufferFunctions* operator->() { return functions_; }
  CommandBufferFunctions& operator*() { return *functions_; }
};

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_COMMAND_BUFFER_WRAPPER_H_
