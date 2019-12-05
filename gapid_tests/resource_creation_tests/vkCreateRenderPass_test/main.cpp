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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->allocator(), &instance, &surface, &queues[0], &queues[1], false));

  vulkan::VkSwapchainKHR swapchain(vulkan::CreateDefaultSwapchain(
      &instance, &device, &surface, data->allocator(), queues[0], queues[1],
      data));

  containers::vector<VkImage> images(data->allocator());
  vulkan::LoadContainer(data->logger(), device->vkGetSwapchainImagesKHR,
                        &images, device, swapchain);

  {  // Test1 0 attachments
    VkSubpassDescription subpass{
        0,                                // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
        0,                                // inputAttachmentCount
        nullptr,                          // pInputAttachments
        0,                                // colorAttachmentCount
        nullptr,                          // pColorAttachments
        nullptr,                          // pResolveAttachments
        nullptr,                          // pDepthStencilAttachment
        0,                                // preserveAttachmentCount
        nullptr                           // pPreserveAttachments
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        0,                                          // attachmentCount
        nullptr,                                    // pAttachments
        1,                                          // subpassCount
        &subpass,                                   // pSubpasses
        0,                                          // dependencyCount
        nullptr                                     // pDependencies
    };

    VkRenderPass raw_render_pass;
    LOG_ASSERT(==, data->logger(),
               device->vkCreateRenderPass(device, &render_pass_create_info,
                                          nullptr, &raw_render_pass),
               VK_SUCCESS);
    // So that it can automatically get cleaned up.
    device->vkDestroyRenderPass(device, raw_render_pass, nullptr);
  }
  {  // Test2 1 attachment

    VkAttachmentDescription color_attachment{
        0,                                 // flags
        VK_FORMAT_R8G8B8A8_UNORM,          // format
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
    LOG_ASSERT(==, data->logger(),
               device->vkCreateRenderPass(device, &render_pass_create_info,
                                          nullptr, &raw_render_pass),
               VK_SUCCESS);
    device->vkDestroyRenderPass(device, raw_render_pass, nullptr);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
