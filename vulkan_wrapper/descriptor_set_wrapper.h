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

#ifndef VULKAN_WRAPPER_DESCRIPTOR_SET_WRAPPER_H_
#define VULKAN_WRAPPER_DESCRIPTOR_SET_WRAPPER_H_

#include <memory>

#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/lazy_function.h"
#include "vulkan_wrapper/sub_objects.h"

namespace vulkan {

// VkDescriptorSet takes the ownership and wraps a native VkDescriptorSet
// object. It will automatically call VkFreeDescriptorSets when it goes
// out of scope.
class VkDescriptorSet {
 public:
  VkDescriptorSet(::VkDescriptorSet set, ::VkDescriptorPool pool,
                  VkDevice* device)
      : descriptor_set_(set),
        pool_(pool),
        device_(*device),
        log_(device->GetLogger()),
        destruction_function_(&(*device)->vkFreeDescriptorSets) {}

  VkDescriptorSet(VkDescriptorSet&& other)
      : descriptor_set_(other.descriptor_set_),
        pool_(other.pool_),
        device_(other.device_),
        log_(other.log_),
        destruction_function_(other.destruction_function_) {
    other.descriptor_set_ = VK_NULL_HANDLE;
  }

  ~VkDescriptorSet() {
    if (descriptor_set_ != VK_NULL_HANDLE) {
      (*destruction_function_)(device_, pool_, 1, &descriptor_set_);
    }
  }

  logging::Logger* GetLogger() { return log_; }

 private:
  ::VkDescriptorSet descriptor_set_;
  ::VkDescriptorPool pool_;
  ::VkDevice device_;
  logging::Logger* log_;
  LazyFunction<PFN_vkFreeDescriptorSets, ::VkDevice, DeviceFunctions>*
      destruction_function_;

 public:
  const ::VkDescriptorSet& get_raw_object() const { return descriptor_set_; }
  operator ::VkDescriptorSet() const { return descriptor_set_; }
};

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_DESCRIPTOR_SET_WRAPPER_H_
