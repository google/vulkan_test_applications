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
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data);
  vulkan::VkDevice& device = app.device();

  {
    // Creates an image with two layers
    VkImageCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_LINEAR,
        /* usage = */ VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };

    auto image = app.CreateAndBindImage(&create_info);

    auto cmdbuf = app.GetCommandBuffer();
    app.BeginCommandBuffer(&cmdbuf);
    vulkan::RecordImageLayoutTransition(
        *image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, &cmdbuf);
    app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&cmdbuf,
                                                     &app.render_queue());

    VkImageSubresource layer{
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        0,                          // mipLevel
        0,                          // arrayLayer
    };
    VkSubresourceLayout layout;
    device->vkGetImageSubresourceLayout(device, *image, &layer, &layout);

    // offset
    LOG_EXPECT(==, data->log, layout.offset, 0);

    // size
    LOG_EXPECT(==, data->log, layout.size, 4 * 32 * 32);

    // rowPitch
    LOG_EXPECT(==, data->log, layout.rowPitch, 4 * 32);

    // arrayPitch
    // arrayPitch is undefined.

    // depthPitch
    LOG_EXPECT(==, data->log, layout.depthPitch, 4 * 32 * 32);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
