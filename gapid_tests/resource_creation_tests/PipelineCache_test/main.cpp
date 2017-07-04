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
#include "vulkan_helpers/structs.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->root_allocator, &wrapper));
  vulkan::VkDevice device(
      vulkan::CreateDefaultDevice(data->root_allocator, instance, false));

  {  // Empty cache
    VkPipelineCacheCreateInfo create_info{
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,  // sType
        nullptr,                                       // pNext
        0,                                             // flags
        0,                                             // initialDataSize
        nullptr                                        // pInitialData
    };

    VkPipelineCache cache;
    LOG_ASSERT(
        ==, data->log,
        device->vkCreatePipelineCache(device, &create_info, nullptr, &cache),
        VK_SUCCESS);
    device->vkDestroyPipelineCache(device, cache, nullptr);
  }

  { // Merge pipeline caches
    VkPipelineCacheCreateInfo create_info{
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,  // sType
        nullptr,                                       // pNext
        0,                                             // flags
        0,                                             // initialDataSize
        nullptr                                        // pInitialData
    };
    VkPipelineCache caches[3];
    LOG_ASSERT(
        ==, data->log,
        device->vkCreatePipelineCache(device, &create_info, nullptr, &caches[0]),
        VK_SUCCESS);
    LOG_ASSERT(
        ==, data->log,
        device->vkCreatePipelineCache(device, &create_info, nullptr, &caches[1]),
        VK_SUCCESS);
    LOG_ASSERT(
        ==, data->log,
        device->vkCreatePipelineCache(device, &create_info, nullptr, &caches[2]),
        VK_SUCCESS);
    LOG_ASSERT(
        ==, data->log,
        device->vkMergePipelineCaches(device, caches[0], 2, &caches[1]),
        VK_SUCCESS);
    device->vkDestroyPipelineCache(device, caches[0], nullptr);
    device->vkDestroyPipelineCache(device, caches[1], nullptr);
    device->vkDestroyPipelineCache(device, caches[2], nullptr);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
