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
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->root_allocator, &instance, &surface, &queues[0], &queues[1]));

  vulkan::VkSwapchainKHR swapchain(vulkan::CreateDefaultSwapchain(
      &instance, &device, &surface, data->root_allocator, queues[0], queues[1],
      data));

  containers::vector<VkImage> images(data->root_allocator);
  vulkan::LoadContainer(data->log.get(), device->vkGetSwapchainImagesKHR,
                        &images, device, swapchain);

  {
    VkAttachmentDescription color_attachment{
        0,                                 // flags
        swapchain.format(),                // format
        VK_SAMPLE_COUNT_1_BIT,             // samples
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   // initialLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR    // finalLayout
    };

    VkAttachmentReference color_attachment_reference{
        0,  // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{
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
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        1,                                          // attachmentCount
        &color_attachment,                          // pAttachments
        1,                                          // subpassCount
        &subpass,                                   // pSubpasses
        0,                                          // dependencyCount
        nullptr                                     // pDependencies
    };

    VkRenderPass raw_render_pass;
    LOG_ASSERT(==, data->log,
               device->vkCreateRenderPass(device, &render_pass_create_info,
                                          nullptr, &raw_render_pass),
               VK_SUCCESS);
    vulkan::VkRenderPass render_pass(raw_render_pass, nullptr, &device);

    VkImageViewCreateInfo image_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        images.front(),                            // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        swapchain.format(),                        // format
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

    VkImageView raw_image_view;
    LOG_EXPECT(==, data->log,
               device->vkCreateImageView(device, &image_view_create_info,
                                         nullptr, &raw_image_view),
               VK_SUCCESS);
    vulkan::VkImageView image_view(raw_image_view, nullptr, &device);

    VkImageView image_views[1] = {image_view};

    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        render_pass,                                // renderPass
        1,                                          // attachmentCount
        image_views,                                // attachments
        swapchain.width(),                          // width
        swapchain.height(),                         // height
        1                                           // layers
    };
    VkFramebuffer framebuffer;
    device->vkCreateFramebuffer(device, &framebuffer_create_info, nullptr,
                                &framebuffer);
    device->vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000)) {
    device->vkDestroyFramebuffer(
        device, static_cast<VkFramebuffer>(VK_NULL_HANDLE), nullptr);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
