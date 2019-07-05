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

#include <algorithm>

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication application(data->allocator(), data->logger(), data,
                                        {}, {}, {0}, 1024 * 100, 1024 * 100,
                                        1024 * 100);

  const VkExtent3D sample_image_extent{32, 32, 1};
  const VkImageCreateInfo sample_image_create_info{
      /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
      /* extent = */ sample_image_extent,
      /* mipLevels = */ 1,
      /* arrayLayers = */ 1,
      /* samples = */ VK_SAMPLE_COUNT_1_BIT,
      /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
      /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
      /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
  };

  {
    // 1. Resolve from a 2D, Optimal tiling, 4x multi-sampled color image, with
    // only 1 layer, 1 miplevel and 0 offsets in all dimensions.
    VkImageCreateInfo src_image_create_info = sample_image_create_info;
    src_image_create_info.samples = VK_SAMPLE_COUNT_4_BIT;  // 4x multi-sampled
    vulkan::ImagePointer src_image =
        application.CreateAndBindImage(&src_image_create_info);
    VkImageCreateInfo dst_image_create_info = sample_image_create_info;
    vulkan::ImagePointer dst_image =
        application.CreateAndBindImage(&dst_image_create_info);
    // Data in the multi-sampled source image
    VkClearColorValue clear_color{
        {0.5, 0.5, 0.5, 0.5}  // uint32[4]
    };
    // Range used for vkCmdClearColorImage() to fill the multi-sampled image
    // with data
    VkImageSubresourceRange clear_color_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
                                              0, 1};

    // Fill the multi-sampled image with data through vkCmdClearColorImage()
    vulkan::VkCommandBuffer cmd_buf = application.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    VkCommandBufferBeginInfo cmd_buf_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);
    vulkan::RecordImageLayoutTransition(*src_image, clear_color_range,
                                        VK_IMAGE_LAYOUT_UNDEFINED, 0,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_ACCESS_TRANSFER_WRITE_BIT, &cmd_buf);
    cmd_buf->vkCmdClearColorImage(cmd_buf, *src_image,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  &clear_color, 1, &clear_color_range);
    // Switch src image layout from TRANSFER_DST to TRANSFER_SRC
    vulkan::RecordImageLayoutTransition(
        *src_image, clear_color_range, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT, &cmd_buf);
    vulkan::RecordImageLayoutTransition(*dst_image, clear_color_range,
                                        VK_IMAGE_LAYOUT_UNDEFINED, 0,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_ACCESS_TRANSFER_WRITE_BIT, &cmd_buf);
    VkImageResolve image_resolve{
        // mip level 0, layerbase 0, layer count 1
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        sample_image_extent,
    };
    // Call vkCmdResolveImage
    cmd_buf->vkCmdResolveImage(
        cmd_buf, *src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dst_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_resolve);
    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             nullptr,
                             0,
                             nullptr,
                             nullptr,
                             1,
                             &raw_cmd_buf,
                             0,
                             nullptr};
    application.render_queue()->vkQueueSubmit(
        application.render_queue(), 1, &submit_info,
        static_cast<VkFence>(VK_NULL_HANDLE));
    application.render_queue()->vkQueueWaitIdle(application.render_queue());

    // Dump the resolved image data
    containers::vector<uint8_t> dump_data(data->allocator());
    application.DumpImageLayersData(
        dst_image.get(),
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},  // subresourceLayer
        {0, 0, 0},                             // offset
        sample_image_extent,                   // extent
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // initial layout
        &dump_data,                            // data
        {}                                     // wait_semaphores
    );
    containers::vector<uint8_t> expected_data(32 * 32 * 4, 0.5 * 255,
                                              data->allocator());
    LOG_ASSERT(==, data->logger(), expected_data.size(), dump_data.size());
    LOG_ASSERT(==, data->logger(), true,
               std::equal(expected_data.begin(), expected_data.end(),
                          dump_data.begin()));
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
