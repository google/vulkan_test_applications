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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  {
    // A query pool with queryCount of value 4, queryType of value
    // VK_QUERY_TYPE_PIPELINE_STATISTICS, and pipelineStatistics of value
    // VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT.

    // Create a new device with pipeline statistics feature enabled from the
    // same physical device.
    VkPhysicalDeviceFeatures request_features = {0};
    request_features.pipelineStatisticsQuery = VK_TRUE;
    vulkan::VulkanApplication application(data->allocator(), data->logger(),
                                          data, {}, request_features);
    if (application.device().is_valid()) {
      vulkan::VkDevice& device = application.device();
      VkQueryPoolCreateInfo query_pool_create_info = {
          VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
          nullptr,                                   // pNext
          0,                                         // flags
          VK_QUERY_TYPE_PIPELINE_STATISTICS,         // queryType
          4,                                         // queryCount
          VkQueryPipelineStatisticFlags(
              VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT),  // pipelineStatistics
      };
      ::VkQueryPool query_pool;
      LOG_EXPECT(==, data->logger(),
                 device->vkCreateQueryPool(device, &query_pool_create_info,
                                           nullptr, &query_pool),
                 VK_SUCCESS);
      device->vkDestroyQueryPool(device, query_pool, nullptr);
    } else {
      data->logger()->LogInfo(
          "Disabled test due to missing physical device feature: "
          "pipelineStatisticsQuery");
    }
  }
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
