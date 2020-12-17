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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();

  const VkImageType type = VK_IMAGE_TYPE_2D;
  const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  const VkDeviceSize pixel_size = 4;  // match with the format
  const VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
  const VkImageUsageFlags usage =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  uint32_t width = 32;
  uint32_t height = 32;
  uint32_t depth = 1;  // becaue of 2D image
  uint32_t mip_level = 6;
  uint32_t array_layer = 3;

  VkImageFormatProperties props;
  app.instance()->vkGetPhysicalDeviceImageFormatProperties(
      app.device().physical_device(), format, type, tiling, usage, 0, &props);
  data->logger()->LogInfo("Physical device image format properties:");
  data->logger()->LogInfo("\tmaxExtent.width: ", props.maxExtent.width);
  data->logger()->LogInfo("\tmaxExtent.height: ", props.maxExtent.height);
  data->logger()->LogInfo("\tmaxExtent.depth: ", props.maxExtent.depth);
  data->logger()->LogInfo("\tmaxMipLevels: ", props.maxMipLevels);
  data->logger()->LogInfo("\tmaxArrayLayers: ", props.maxArrayLayers);
  data->logger()->LogInfo("\tsampleCounts: ", props.sampleCounts);

  width = props.maxExtent.width < width ? props.maxExtent.width : width;
  height = props.maxExtent.height < height ? props.maxExtent.height : height;
  depth = props.maxExtent.depth < depth ? props.maxExtent.depth : depth;
  mip_level = props.maxMipLevels < mip_level ? props.maxMipLevels : mip_level;
  array_layer =
      props.maxArrayLayers < array_layer ? props.maxArrayLayers : array_layer;

  if (width == 0 || height == 0 || depth == 0 || mip_level == 0 ||
      array_layer == 0 || (props.sampleCounts & VK_SAMPLE_COUNT_1_BIT) == 0) {
    data->logger()->LogInfo(
        "Linear tiling image with following type/format/usage/sample not "
        "supported");
    data->logger()->LogInfo("\tformat: ", format);
    data->logger()->LogInfo("\ttype: ", type);
    data->logger()->LogInfo("\tusage: ", usage);
    data->logger()->LogInfo("\tsample count: VK_SAMPLE_COUNT_1_BIT");
    return 0;
  }

  {
    // Creates an image with two layers
    VkImageCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ type,
        /* format = */ format,
        /* extent = */
        {
            /* width = */ width,
            /* height = */ height,
            /* depth = */ depth,
        },
        /* mipLevels = */ mip_level,
        /* arrayLayers = */ array_layer,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_LINEAR,
        /* usage = */ usage,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_PREINITIALIZED,
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

    uint32_t query_level = mip_level > 1 ? 1 : 0;
    uint32_t query_layer = array_layer > 1 ? 1 : 0;
    VkImageSubresource layer{
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        query_level,                // mipLevel
        query_layer,                // arrayLayer
    };
    VkSubresourceLayout layout;
    device->vkGetImageSubresourceLayout(device, *image, &layer, &layout);

    VkMemoryRequirements req;
    device->vkGetImageMemoryRequirements(device, *image, &req);

    data->logger()->LogInfo("Image subresource layout info:");
    data->logger()->LogInfo("\tMip level: ", query_level);
    data->logger()->LogInfo("\tArray layer: ", query_layer);
    data->logger()->LogInfo("\tSubresourceLayout.offset: ", layout.offset);
    data->logger()->LogInfo("\tSubresourceLayout.size: ", layout.size);
    data->logger()->LogInfo("\tSubresourceLayout.rowPitch: ", layout.rowPitch);
    data->logger()->LogInfo("\tSubresourceLayout.arrayPitch: ",
                            layout.arrayPitch);
    data->logger()->LogInfo("\tSubresourceLayout.depthPitch: ",
                            layout.depthPitch);

    // offset, must be within the valid range.
    LOG_EXPECT(<=, data->logger(), layout.offset + layout.size, req.size);

    uint32_t mip_width =
        width / (1 << query_level) == 0 ? 1 : width / (1 << query_level);
    uint32_t mip_height =
        height / (1 << query_level) == 0 ? 1 : height / (1 << query_level);
    uint32_t mip_depth =
        depth / (1 << query_level) == 0 ? 1 : depth / (1 << query_level);

    // size, must be greater than or equal to a tightly packed pixel data blob.
    LOG_EXPECT(>=, data->logger(), layout.size,
               pixel_size * mip_width * mip_height * mip_depth);

    // rowPitch, must be greater than or equal to a tightly packed 1D row.
    LOG_EXPECT(>=, data->logger(), layout.rowPitch, pixel_size * mip_width);

    if (depth > 1) {
      // depthPitch, must be greater than or equal to a tightly packed 2D slice.
      LOG_EXPECT(>=, data->logger(), layout.depthPitch,
                 pixel_size * mip_width * mip_height);
    }

    if (array_layer > 1) {
      // arrayPitch, must be greater than or equal to a tightly packed 3D
      // subresource. But there is no guarantee that one layer contains multiple
      // mip levels, nor the other way around.
      LOG_EXPECT(>=, data->logger(), layout.arrayPitch,
                 pixel_size * mip_width * mip_height);
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
