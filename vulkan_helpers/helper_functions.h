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

#ifndef VULKAN_HELPERS_HELPER_FUNCTIONS_H_
#define VULKAN_HELPERS_HELPER_FUNCTIONS_H_

#include <tuple>

#include "support/containers/vector.h"
#include "support/entry/entry.h"
#include "vulkan_wrapper/command_buffer_wrapper.h"
#include "vulkan_wrapper/descriptor_set_wrapper.h"
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/queue_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"
#include "vulkan_wrapper/swapchain.h"

namespace vulkan {

// Clears the memory of the given object
template <typename T>
void ZeroMemory(T* val) {
  memset(val, 0x00, sizeof(T));
}

// Create an empty instance. Vulkan functions that are resolved by the created
// instance will be stored in the space allocated by the given |allocator|. The
// |allocator| must continue to exist until the instance is destroied.
VkInstance CreateEmptyInstance(containers::Allocator* allocator,
                               LibraryWrapper* _wrapper);

// Creates an instance with the swapchain and surface layers enabled. Vulkan
// functions that are resolved through the created instance will be stored in
// the space allocated by the given |allocator|. The |allocator| must continue
// to exist until the instance is destroied.
VkInstance CreateDefaultInstance(containers::Allocator* allocator,
                                 LibraryWrapper* _wrapper);

// Creates an instance with either a real or virtual swapchain based on
// whether or not data requests an external swapchain. Otherwise
// identical to CreateDefaultInstance.
VkInstance CreateInstanceForApplication(containers::Allocator* allocator,
                                        LibraryWrapper* wrapper,
                                        const entry::entry_data* data);

containers::vector<VkPhysicalDevice> GetPhysicalDevices(
    containers::Allocator* allocator, VkInstance& instance);

// Gets all queue family properties for the given physical |device| from the
// given |instance|. Queue family properties will be returned in a vector
// allocated from the given |allocator|.
containers::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(
    containers::Allocator* allocator, VkInstance& instance,
    ::VkPhysicalDevice device);

// Returns the index for the first queue family with both graphics and compute
// capabilities for the given physical |device|. Returns the max unit32_t value
// if no such queue.
uint32_t GetGraphicsAndComputeQueueFamily(containers::Allocator* allocator,
                                          VkInstance& instance,
                                          ::VkPhysicalDevice device);

// Creates a device from the given |instance| with one queue. If
// |require_graphics_and_compute_queue| is true, the queue is of both graphics
// and compute capabilities. Vulkan functions that are resolved through the
// create device will be stored in the space allocated by the given |allocator|.
// The |allocator| must continue to exist until the device is destroyed.
VkDevice CreateDefaultDevice(containers::Allocator* allocator,
                             VkInstance& instance,
                             bool require_graphics_compute_queue = false);

// Creates a command pool with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
// set.
VkCommandPool CreateDefaultCommandPool(containers::Allocator* allocator,
                                       VkDevice& device);

// Creates a surface to render into the the default window
// provided in entry_data.
VkSurfaceKHR CreateDefaultSurface(VkInstance* instance,
                                  const entry::entry_data* entry_data);

// Creates a device capable of presenting to the given surface.
// The device is created with the given extensions.
// If the given extensions do not exist, an invalid device is returned.
// Returns the queue indices for the present and graphics queues.
// If try_to_find_separate_present_queue is true, then the engine will
// try to allocate a distinct queue for presentation. It will try to do
// so as a separate queue family.
// If async_compute_queue_index is not nullptr, then the device will be
// created with an async compute queue if valid. This will fill in the
// async_compute_queue_index with the queue family of the compute queue.
// If no async compute queue could be created, *async_compute_queue_index
// will be 0xFFFFFFFF
// Note: They may be the same or different.
VkDevice CreateDeviceForSwapchain(
    containers::Allocator* allocator, VkInstance* instance,
    VkSurfaceKHR* surface, uint32_t* present_queue_index,
    uint32_t* graphics_queue_index,
    const std::initializer_list<const char*> extensions = {},
    const VkPhysicalDeviceFeatures& features = {0},
    bool try_to_find_separate_present_queue = false,
    uint32_t* aync_compute_queue_index = nullptr);

// Creates a primary level default command buffer from the given command pool
// and the device.
VkCommandBuffer CreateDefaultCommandBuffer(VkCommandPool* pool,
                                           VkDevice* device);

// Creates a command buffer from the given command pool, the given device with
// the specified command buffer level.
VkCommandBuffer CreateCommandBuffer(VkCommandPool* pool,
                                    VkCommandBufferLevel level,
                                    VkDevice* device);

// Creates a swapchain with a default layout and number of images.
// It will be able to be rendered to from graphics_queue_index,
// and it will be presentable on present_queue_index.
VkSwapchainKHR CreateDefaultSwapchain(VkInstance* instance, VkDevice* device,
                                      VkSurfaceKHR* surface,
                                      containers::Allocator* allocator,
                                      uint32_t present_queue_index,
                                      uint32_t graphics_queue_index,
                                      const entry::entry_data* data);

// Returns a uint32_t with only the lowest bit set.
uint32_t inline GetLSB(uint32_t val) { return ((val - 1) ^ val) & val; }

// Creates a 2D color-attachment R8G8B8A8 unorm format image with the specified
// width and height. The image is not multi-sampled and is in exclusive sharing
// mode. Its mipLevels and arrayLayers are set to 1, its image tiling is set to
// VK_IMAGE_TILING_OPTIMAL and its initialLayout is set to
// VK_IMAGE_LAYOUT_UNDEFINED.
VkImage CreateDefault2DColorImage(VkDevice* device, uint32_t width,
                                  uint32_t height);

// Creates a default sampler with normalized coordinates. magFilter, minFilter,
// and mipmap are all using nearest mode. Addressing modes for U, V, and W
// coordinates are all clamp-to-edge. mipLodBias, minLod, and maxLod are all 0.
// anisotropy and compare is disabled.
VkSampler CreateDefaultSampler(VkDevice* device);

// Creates a default sampler as above, but with the specified minFilter and
// magFilter.
VkSampler CreateSampler(VkDevice* device, VkFilter minFilter,
                        VkFilter magFilter);

// Creates a DescriptorSetLayout with the given set of layouts.
VkDescriptorSetLayout CreateDescriptorSetLayout(
    containers::Allocator* allocator, VkDevice* device,
    std::initializer_list<VkDescriptorSetLayoutBinding> bindings);

// Creates a descriptor pool with the given pool sizes. At maximum, |max_sets|
// number of descriptor sets can be allocated from this pool.
VkDescriptorPool CreateDescriptorPool(VkDevice* device, uint32_t num_pool_size,
                                      const VkDescriptorPoolSize* pool_sizes,
                                      uint32_t max_sets);

// Creates a descriptor set layout with |count| descriptors of the given |type|
// bound to bining number 0. The resource behind the descriptors is set to
// available to all shader stages.
VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice* device,
                                                ::VkDescriptorType type,
                                                uint32_t count);

// Allocates a descriptor set with the given |layout| from the given |pool|.
VkDescriptorSet AllocateDescriptorSet(VkDevice* device, ::VkDescriptorPool pool,
                                      ::VkDescriptorSetLayout layout);

// Allocates device memory of the given |size| from the given
// |memory_type_index|.
VkDeviceMemory AllocateDeviceMemory(VkDevice* device,
                                    uint32_t memory_type_index,
                                    ::VkDeviceSize size);

// Creates a shader module out of the given SPIR-V |words|.
template <unsigned size>
VkShaderModule CreateShaderModule(VkDevice* device,
                                  const uint32_t (&words)[size]) {
  const ::VkShaderModuleCreateInfo create_info = {
      /* sType = */ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* codeSize = */ 4 * size,
      /* pCode = */ words,
  };
  ::VkShaderModule raw_shader_module;
  LOG_ASSERT(==, device->GetLogger(), VK_SUCCESS,
             (*device)->vkCreateShaderModule(*device, &create_info, nullptr,
                                             &raw_shader_module));
  return VkShaderModule(raw_shader_module, nullptr, device);
}

// Returns the "index" queue from the given queue_family.
VkQueue inline GetQueue(VkDevice* device, uint32_t queue_family_index,
                        uint32_t index = 0) {
  ::VkQueue queue;
  (*device)->vkGetDeviceQueue(*device, queue_family_index, index, &queue);
  return VkQueue(queue, device, queue_family_index);
}

// Creates a default pipeline cache, it does not load anything from disk.
VkPipelineCache CreateDefaultPipelineCache(VkDevice* device);

// Creates a query pool with the given query pool create info from the given
// device if the given device is valid. Otherwise returns a query pool with
// VK_NULL_HANDLE inside.
VkQueryPool CreateQueryPool(VkDevice* device,
                            const VkQueryPoolCreateInfo& create_info);

// Given a bitmask of required_index_bits and a bitmask of
// VkMemoryPropertyFlags, return the first memory index
// from the given device that supports the required_property_flags.
// Will assert if one could not be found.
uint32_t inline GetMemoryIndex(VkDevice* device, logging::Logger* log,
                               uint32_t required_index_bits,
                               VkMemoryPropertyFlags required_property_flags) {
  const VkPhysicalDeviceMemoryProperties& properties =
      device->physical_device_memory_properties();
  LOG_ASSERT(<=, log, properties.memoryTypeCount, 32);
  uint32_t memory_index = 0;
  for (; memory_index < properties.memoryTypeCount; ++memory_index) {
    if (!(required_index_bits & (1 << memory_index))) {
      continue;
    }

    if ((properties.memoryTypes[memory_index].propertyFlags &
         required_property_flags) != required_property_flags) {
      continue;
    }
    break;
  }
  LOG_ASSERT(!=, log, memory_index, properties.memoryTypeCount);
  return memory_index;
}

// Changes the layout of the given |image| with the specified
// |subresource_range| from |old_layout| with access mask |src_access_mask| to
// |new_layout| with access mask |dst_access_mask| through the given command
// buffer |cmd_buffer|. If a |queue| is given, |cmd_buffer| will be submitted
// to the given |queue| and the execution will wait until the |wait_semaphores|
// signal to begin and signal the |signal_semaphores| and |fence| once
// finished.
void SetImageLayout(::VkImage image,
                    const VkImageSubresourceRange& subresource_range,
                    VkImageLayout old_layout, VkAccessFlags src_access_mask,
                    VkImageLayout new_layout, VkAccessFlags dst_access_mask,
                    VkCommandBuffer* cmd_buffer, VkQueue* queue,
                    std::initializer_list<::VkSemaphore> wait_semaphores,
                    std::initializer_list<::VkSemaphore> signal_semaphores,
                    ::VkFence fence, containers::Allocator* allocator);

// Returns a tuple of three uint_32 values: element size in bytes, texel block
// width and height in pixel, for the given format. Returns a tuple with all
// zero values if the given format is not recognized.
std::tuple<uint32_t, uint32_t, uint32_t> GetElementAndTexelBlockSize(
    VkFormat format);

inline VkFence CreateFence(VkDevice* device, bool signaled = false) {
  ::VkFence raw_fence_ = VK_NULL_HANDLE;
  VkFenceCreateInfo create_info = {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,                             // sType
      nullptr,                                                         // pNext
      signaled ? VkFenceCreateFlags(VK_FENCE_CREATE_SIGNALED_BIT) : 0  // flags
  };
  LOG_ASSERT(
      ==, device->GetLogger(), VK_SUCCESS,
      (*device)->vkCreateFence(*device, &create_info, nullptr, &raw_fence_));
  return VkFence(raw_fence_, nullptr, device);
}

inline VkSemaphore CreateSemaphore(VkDevice* device) {
  ::VkSemaphore raw_semaphore_ = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo create_info = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
      nullptr,                                  // pNext
      0,                                        // flags
  };
  LOG_ASSERT(==, device->GetLogger(), VK_SUCCESS,
             (*device)->vkCreateSemaphore(*device, &create_info, nullptr,
                                          &raw_semaphore_));
  return VkSemaphore(raw_semaphore_, nullptr, device);
}

// Returns the size of the given image extent specified through width, height,
// depth and format. Returns 0 a valid size is not available, e.g. the given
// format is not recognized.
size_t GetImageExtentSizeInBytes(const VkExtent3D& extent, VkFormat format);

// Returns true if all the request features are supported by the given physical
// device, otherwise returns false. The supported features are returned from
// Vulkan command vkGetPhysicalDeviceFeatures, the command is resolved by the
// given instance.
bool SupportRequestPhysicalDeviceFeatures(
    VkInstance* instance, VkPhysicalDevice physical_device,
    const VkPhysicalDeviceFeatures& request_features);

// Runs the given call once with a nullptr value, and gets the numerical result.
// Resizes the given array, and runs the call again to fill the array.
// Asserts that the function call succeeded.
template <typename Function, typename... Args, typename ContainedType>
void LoadContainer(logging::Logger* log, Function& fn,
                   containers::vector<ContainedType>* ret_val, Args&... args) {
  uint32_t num_values = 0;
  LOG_ASSERT(==, log, (fn)(args..., &num_values, nullptr), VK_SUCCESS);
  ret_val->resize(num_values);
  LOG_ASSERT(==, log, (fn)(args..., &num_values, ret_val->data()), VK_SUCCESS);
}
}  // namespace vulkan

#endif  //  VULKAN_HELPERS_HELPER_FUNCTIONS_H_
