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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication application(data->allocator(), data->logger(), data,
                                        vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = application.device();

  {
    // 1. Clear a 32 x 32 rect of a single-layer color attachment in a render
    // pass with one color attachments, one subpasses and no dependencies.
    // Create render pass
    VkAttachmentReference color_attachment_reference{
        0,                                         // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // layout
    };
    vulkan::VkRenderPass render_pass = application.CreateRenderPass(
        {{
            0,                                 // flags
            application.swapchain().format(),  // format
            VK_SAMPLE_COUNT_1_BIT,             // samples
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,  // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR    // finalLayout
        }},
        {{
            0,                                // flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
            0,                                // inputAttachmentCount
            nullptr,                          // pInputAttachments
            1,                                // colorAttachmentCount
            &color_attachment_reference,      // pColorAttachments
            nullptr,                          // pResolveAttachments
            nullptr,                          // pDepthStencilAttachment
            0,                                // preserveAttachmentCount
            nullptr                           // pPreserveAttachments
        }},
        {});

    // Create image view
    VkImageViewCreateInfo image_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        application.swapchain_images().front(),    // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        application.swapchain().format(),          // format
        {
            VK_COMPONENT_SWIZZLE_IDENTITY,  // components.r
            VK_COMPONENT_SWIZZLE_IDENTITY,  // components.g
            VK_COMPONENT_SWIZZLE_IDENTITY,  // components.b
            VK_COMPONENT_SWIZZLE_IDENTITY,  // components.a
        },
        {
            VK_IMAGE_ASPECT_COLOR_BIT,  // subresourceRange.aspectMask
            0,                          // subresourceRange.baseMipLevel
            1,                          // subresourceRange.levelCount
            0,                          // subresourceRange.baseArrayLayer
            1,                          // subresourceRange.layerCount
        },
    };
    ::VkImageView raw_image_view;
    LOG_EXPECT(==, data->logger(),
               device->vkCreateImageView(device, &image_view_create_info,
                                         nullptr, &raw_image_view),
               VK_SUCCESS);
    vulkan::VkImageView image_view(raw_image_view, nullptr, &device);

    // Create framebuffer
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        render_pass,                                // renderPass
        1,                                          // attachmentCount
        &raw_image_view,                            // attachments
        application.swapchain().width(),            // width
        application.swapchain().height(),           // height
        1                                           // layers
    };
    ::VkFramebuffer raw_framebuffer;
    device->vkCreateFramebuffer(device, &framebuffer_create_info, nullptr,
                                &raw_framebuffer);
    vulkan::VkFramebuffer framebuffer(raw_framebuffer, nullptr, &device);
    vulkan::VkCommandBuffer command_buffer = application.GetCommandBuffer();
    VkCommandBufferBeginInfo command_buffer_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr,                                      // pInheritanceInfo
    };
    command_buffer->vkBeginCommandBuffer(command_buffer,
                                         &command_buffer_begin_info);
    VkRenderPassBeginInfo render_pass_begin_info{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        render_pass,                               // renderPass
        framebuffer,                               // framebuffer
        {
            0,                                 // renderArea.offset.x
            0,                                 // renderArea.offset.y
            application.swapchain().width(),   // renderArea.extent.width
            application.swapchain().height(),  // renderArea.extent.height
        },
        0,        // clearValueCount
        nullptr,  // pClearValues
    };
    command_buffer->vkCmdBeginRenderPass(
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Call vkCmdClearAttachments
    VkClearRect clear_rect{
        {
            {0, 0},   // offset
            {32, 32}  // extent
        },            // rect
        0,            // baseArrayLayer
        1,            // layerCount
    };
    VkClearValue clear_value{};
    clear_value.color.float32[0] = 0.2;
    clear_value.color.float32[1] = 0.2;
    clear_value.color.float32[2] = 0.2;
    clear_value.color.float32[3] = 0.2;
    VkClearAttachment clear_attachment{
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        0,                          // colorAttachment
        clear_value                 // clearValue
    };
    command_buffer->vkCmdClearAttachments(command_buffer, 1, &clear_attachment,
                                          1, &clear_rect);

    command_buffer->vkCmdEndRenderPass(command_buffer);
    command_buffer->vkEndCommandBuffer(command_buffer);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
