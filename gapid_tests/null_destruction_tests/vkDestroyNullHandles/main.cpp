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

#include "support/containers/vector.h"
#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  containers::vector<VkPhysicalDevice> devices(
      vulkan::GetPhysicalDevices(data->root_allocator, instance),
      data->root_allocator);

  float priority = 1.f;
  VkDeviceQueueCreateInfo queue_info{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
      nullptr,                                     // pNext
      0,                                           // flags
      0,                                           // queueFamilyIndex
      1,                                           // queueCount
      &priority                                    // pQueuePriorities
  };

  VkDeviceCreateInfo info{
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  // sType
      nullptr,                               // pNext
      0,                                     // flags
      0,                                     // queueCreateInfoCount
      nullptr,                               // pQueueCreateInfo
      0,                                     // enabledLayerNames
      nullptr,                               // ppEnabledLayerNames
      0,                                     // enabledExtensionCount
      nullptr,                               // ppEnabledextensionNames
      nullptr                                // pEnalbedFeatures
  };

  {
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue_info;
    ::VkDevice raw_device;
    LOG_EXPECT(==, data->log, instance->vkCreateDevice(devices[0], &info,
                                                       nullptr, &raw_device),
               VK_SUCCESS);

    vulkan::VkDevice device(
        vulkan::CreateDefaultDevice(data->root_allocator, instance, false));

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000) &&
        NOT_DEVICE(data->log.get(), device, vulkan::Nvidia965M, 0x5c4f4000)) {
      device->vkDestroyPipelineCache(
          device, static_cast<VkPipelineCache>(VK_NULL_HANDLE), nullptr);
      VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};
      vulkan::VkDescriptorPool pool =
          CreateDescriptorPool(&device, 1, &pool_size, 1);
      ::VkDescriptorPool raw_pool = pool.get_raw_object();

      ::VkDescriptorSet sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
      device->vkFreeDescriptorSets(device, raw_pool, 2, sets);

      device->vkDestroyBuffer(device, (VkBuffer)VK_NULL_HANDLE, nullptr);

      device->vkDestroyBufferView(device, (VkBufferView)VK_NULL_HANDLE,
                                  nullptr);

      device->vkDestroyDescriptorSetLayout(
          device, (VkDescriptorSetLayout)VK_NULL_HANDLE, nullptr);

      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);

      device->vkDestroyImageView(device, (VkImageView)VK_NULL_HANDLE, nullptr);

      device->vkDestroyQueryPool(device, (VkQueryPool)VK_NULL_HANDLE, nullptr);
      device->vkDestroySampler(device, (VkSampler)VK_NULL_HANDLE, nullptr);
      device->vkDestroyDescriptorPool(device, (VkDescriptorPool)VK_NULL_HANDLE,
                                      nullptr);
      device->vkFreeMemory(device, VkDeviceMemory(VK_NULL_HANDLE), nullptr);
      device->vkDestroyPipelineCache(
          device, static_cast<VkPipelineCache>(VK_NULL_HANDLE), nullptr);

      device->vkDestroySemaphore(device, (VkSemaphore)VK_NULL_HANDLE, nullptr);

      device->vkDestroyFramebuffer(
          device, static_cast<VkFramebuffer>(VK_NULL_HANDLE), nullptr);
      device->vkDestroyPipelineLayout(
          device, static_cast<VkPipelineLayout>(VK_NULL_HANDLE), nullptr);

      device->vkDestroyRenderPass(
          device, static_cast<VkRenderPass>(VK_NULL_HANDLE), nullptr);

      device->vkDestroyShaderModule(
          device, static_cast<VkShaderModule>(VK_NULL_HANDLE), nullptr);

      device->vkDestroyFence(device, (VkFence)VK_NULL_HANDLE, nullptr);

      // These things also fail on Android 7.1
      if (NOT_ANDROID_VERSION(data, "7.1.1")) {
        device->vkDestroySwapchainKHR(device, (VkSwapchainKHR)VK_NULL_HANDLE,
                                      nullptr);
      }
    }
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
