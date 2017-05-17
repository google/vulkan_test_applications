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

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/structs.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;
  vulkan::LibraryWrapper wrapper(allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(allocator, instance));
  vulkan::VkCommandPool pool(
      vulkan::CreateDefaultCommandPool(allocator, device));

  const uint32_t max_count = 5;

  containers::vector<::VkCommandBufferResetFlags> reset_flags =
      vulkan::AllVkCommandBufferResetFlagCombinations(allocator);
  uint32_t reset_flag_index = reset_flags.size() - 1;

  containers::vector<::VkCommandBuffer> command_buffers(allocator);
  command_buffers.reserve(max_count);
  for (VkCommandBufferLevel level :
       vulkan::AllVkCommandBufferLevels(allocator)) {
    for (uint32_t count : {uint32_t(0), uint32_t(1), max_count}) {
      data->log->LogInfo("commandBufferLevel: ", level);
      data->log->LogInfo("commandBufferCount: ", count);

      data->log->LogInfo("  API: vkAllocateCommandBuffers");
      const VkCommandBufferAllocateInfo create_info = {
          /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          /* pNext = */ nullptr,
          /* commandPool = */ pool.get_raw_object(),
          /* level = */ level,
          /* commandBufferCount = */ count,
      };
      LOG_EXPECT(==, data->log,
                 device->vkAllocateCommandBuffers(device, &create_info,
                                                  command_buffers.data()),
                 VK_SUCCESS);
      for (uint32_t i = 0; i < count; ++i) {
        data->log->LogInfo("    handle: ", command_buffers[i]);
      }

      data->log->LogInfo("  API: vkResetCommandBuffer");
      for (uint32_t i = 0; i < count; ++i) {
        LOG_EXPECT(==, data->log,
                   device->command_buffer_functions()->vkResetCommandBuffer(
                       command_buffers[i],
                       // Use reset flags in a circular way.
                       (reset_flag_index =
                            (reset_flag_index + 1) % reset_flags.size())),
                   VK_SUCCESS);
      }

      data->log->LogInfo("  API: vkFreeCommandBuffers");
      device->vkFreeCommandBuffers(device, pool.get_raw_object(), count,
                                   command_buffers.data());
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
