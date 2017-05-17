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

#ifndef VULKAN_WRAPPER_QUEUE_WRAPPER_H
#define VULKAN_WRAPPER_QUEUE_WRAPPER_H

#include "vulkan_wrapper/sub_objects.h"

namespace vulkan {

struct QueueTraits {
  using type = ::VkQueue;
  static void dummy_destruction_function(::VkDevice, ::VkQueue,
                                         ::VkAllocationCallbacks*) {}
  using destruction_function_pointer_type = void (*)(::VkDevice, ::VkQueue,
                                                     ::VkAllocationCallbacks*);
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions*) {
    return &dummy_destruction_function;
  }
};

class VkQueue : public VkSubObject<QueueTraits, DeviceTraits> {
 public:
  VkQueue(::VkQueue queue, VkDevice* device, uint32_t index)
      : VkSubObject<QueueTraits, DeviceTraits>(queue, nullptr, device),
        functions_((*device)->queue_functions()),
        queue_family_index_(index) {}

  QueueFunctions* operator->() { return functions_; }
  QueueFunctions& operator*() { return *functions_; }

  uint32_t index() const { return queue_family_index_; }

 private:
  QueueFunctions* functions_;
  uint32_t queue_family_index_;
};
}  // namespace vulkan
#endif  // VULKAN_WRAPPER_QUEUE_WRAPPER_H
