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

#ifndef VULKAN_WRAPPER_SWAPCHAIN_H
#define VULKAN_WRAPPER_SWAPCHAIN_H

#include "vulkan_wrapper/sub_objects.h"

namespace vulkan {
struct SwapchainTraits {
  using type = ::VkSwapchainKHR;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroySwapchainKHR>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroySwapchainKHR;
  }
};

class VkSwapchainKHR : public VkSubObject<SwapchainTraits, DeviceTraits> {
 public:
  VkSwapchainKHR(::VkSwapchainKHR swapchain, VkAllocationCallbacks* allocator,
                 VkDevice* device, uint32_t width, uint32_t height,
                 uint32_t depth, VkFormat format)
      : VkSubObject<SwapchainTraits, DeviceTraits>(swapchain, allocator,
                                                   device),
        format_(format),
        width_(width),
        height_(height),
        depth_(depth) {}

  VkSwapchainKHR(VkSwapchainKHR&& other)
      : VkSubObject<SwapchainTraits, DeviceTraits>(std::move(other)),
        format_(other.format_),
        width_(other.width_),
        height_(other.height_),
        depth_(other.depth_) {}

  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }
  uint32_t depth() const { return depth_; }
  VkFormat format() const { return format_; }

 private:
  VkFormat format_;
  uint32_t width_;
  uint32_t height_;
  uint32_t depth_;
};
}  // namespace vulkan

#endif  //  VULKAN_WRAPPER_SWAPCHAIN_H