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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  vulkan::VkDevice device(
      vulkan::CreateDefaultDevice(data->root_allocator, instance));

  {
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 0};

    ::VkCommandPool raw_command_pool;
    LOG_ASSERT(==, data->log,
               device->vkCreateCommandPool(device, &pool_info, nullptr,
                                           &raw_command_pool),
               VK_SUCCESS);
    vulkan::VkCommandPool command_pool(raw_command_pool, nullptr, &device);
    LOG_ASSERT(==, data->log, VK_SUCCESS,
               device->vkResetCommandPool(device, command_pool, 0));
  }

  {
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 0};

    ::VkCommandPool raw_command_pool;
    LOG_ASSERT(==, data->log,
               device->vkCreateCommandPool(device, &pool_info, nullptr,
                                           &raw_command_pool),
               VK_SUCCESS);
    vulkan::VkCommandPool command_pool(raw_command_pool, nullptr, &device);
    LOG_ASSERT(==, data->log, VK_SUCCESS,
               device->vkResetCommandPool(device, command_pool, 0));
  }

  {
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        0};

    ::VkCommandPool raw_command_pool;
    LOG_ASSERT(==, data->log,
               device->vkCreateCommandPool(device, &pool_info, nullptr,
                                           &raw_command_pool),
               VK_SUCCESS);
    vulkan::VkCommandPool command_pool(raw_command_pool, nullptr, &device);
    LOG_ASSERT(
        ==, data->log, VK_SUCCESS,
        device->vkResetCommandPool(device, command_pool,
                                   VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
  }

  {
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, 0};

    ::VkCommandPool raw_command_pool;
    LOG_ASSERT(==, data->log,
               device->vkCreateCommandPool(device, &pool_info, nullptr,
                                           &raw_command_pool),
               VK_SUCCESS);
    vulkan::VkCommandPool command_pool(raw_command_pool, nullptr, &device);
    LOG_ASSERT(
        ==, data->log, VK_SUCCESS,
        device->vkResetCommandPool(device, command_pool,
                                   VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
