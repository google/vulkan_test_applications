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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;

  vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data);
  {  // 1. Sampler using normalized coordinates
    vulkan::VkDevice& device = app.device();
    VkSamplerCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* magFilter = */ VK_FILTER_NEAREST,
        /* minFilter = */ VK_FILTER_LINEAR,
        /* mipmapMode = */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
        /* addressModeU = */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        /* addressModeV = */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* addressModeW = */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        /* mipLodBias = */ -1.f,
        /* anisotropyEnable = */ false,
        /* maxAnisotropy = */ 1.f,
        /* compareEnable = */ false,
        /* compareOp = */ VK_COMPARE_OP_ALWAYS,
        /* minLod = */ 1.f,
        /* maxLod = */ 2.f,
        /* borderColor = */ VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        /* unnormalizedCoordinates = */ false,
    };
    ::VkSampler sampler;
    device->vkCreateSampler(device, &create_info, nullptr, &sampler);
    data->log->LogInfo("  sampler: ", sampler);
    device->vkDestroySampler(device, sampler, nullptr);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
