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
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::VulkanApplication application(data->root_allocator, data->log.get(),
                                        data);

  {
    // 1. A query pool with queryCount of value 1, queryType of value
    // VK_QUERY_TYPE_OCCLUSION.
    vulkan::VkDevice& device = application.device();
    VkQueryPoolCreateInfo query_pool_create_info = {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        VK_QUERY_TYPE_OCCLUSION,                   // queryType
        1,                                         // queryCount
        0,                                         // pipelineStatistics
    };
    ::VkQueryPool query_pool;
    LOG_EXPECT(==, data->log,
               device->vkCreateQueryPool(device, &query_pool_create_info,
                                         nullptr, &query_pool),
               VK_SUCCESS);
    device->vkDestroyQueryPool(device, query_pool, nullptr);
  }

  {
    // 2. A query pool with queryCount of value 7, queryType of value
    // VK_QUERY_TYPE_TIMESTAMP.
    vulkan::VkDevice& device = application.device();
    VkQueryPoolCreateInfo query_pool_create_info = {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        VK_QUERY_TYPE_TIMESTAMP,                   // queryType
        7,                                         // queryCount
        0,                                         // pipelineStatistics
    };
    ::VkQueryPool query_pool;
    LOG_EXPECT(==, data->log,
               device->vkCreateQueryPool(device, &query_pool_create_info,
                                         nullptr, &query_pool),
               VK_SUCCESS);
    device->vkDestroyQueryPool(device, query_pool, nullptr);
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
