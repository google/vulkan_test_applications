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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/queue_wrapper.h"

uint32_t fragment_shader[] =
#include "simple_fragment.frag.spv"
    ;

uint32_t vertex_shader[] =
#include "simple_vertex.vert.spv"
    ;

vulkan::VulkanGraphicsPipeline CreatePipelineWithDepthBoundTestEnabled(
    const entry::EntryData* data, vulkan::VulkanApplication* app_ptr) {
  LOG_ASSERT(!=, data->logger(), 0, (uintptr_t)app_ptr);
  vulkan::VulkanApplication& app = *app_ptr;
  vulkan::PipelineLayout pipeline_layout(app.CreatePipelineLayout(
      {{{
            0,                                  // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
            1,                                  // descriptorCount
            VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
            nullptr,                            // pImmutableSamplers
        },
        {
            1,                                          // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // descriptorType
            1,                                          // descriptorCount
            VK_SHADER_STAGE_FRAGMENT_BIT,               // stageFlags
            nullptr,                                    // pImmutableSamplers
        }}}));

  VkAttachmentReference color_attachment = {
      1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depth_attachment = {
      0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  vulkan::VkRenderPass render_pass = app.CreateRenderPass(
      {{
           0,                                                 // flags
           VK_FORMAT_D32_SFLOAT,                              // format
           VK_SAMPLE_COUNT_1_BIT,                             // samples
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,                   // loadOp
           VK_ATTACHMENT_STORE_OP_STORE,                      // storeOp
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,                   // stenilLoadOp
           VK_ATTACHMENT_STORE_OP_DONT_CARE,                  // stenilStoreOp
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL   // finalLayout
       },
       {
           0,                                         // flags
           VK_FORMAT_R8G8B8A8_UNORM,                  // format
           VK_SAMPLE_COUNT_1_BIT,                     // samples
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // loadOp
           VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
           VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
       }},                                            // AttachmentDescriptions
      {{
          0,                                // flags
          VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
          0,                                // inputAttachmentCount
          nullptr,                          // pInputAttachments
          1,                                // colorAttachmentCount
          &color_attachment,                // colorAttachment
          nullptr,                          // pResolveAttachments
          &depth_attachment,                // pDepthStencilAttachment
          0,                                // preserveAttachmentCount
          nullptr                           // pPreserveAttachments
      }},                                   // SubpassDescriptions
      {}                                    // SubpassDependencies
  );

  vulkan::VulkanGraphicsPipeline pipeline =
      app.CreateGraphicsPipeline(&pipeline_layout, &render_pass, 0);
  pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", vertex_shader);
  pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragment_shader);
  // Dynamic depth bounds
  pipeline.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
  pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline.AddInputStream(4 * 6,  // vec4 + vec2
                          VK_VERTEX_INPUT_RATE_VERTEX,
                          {
                              {0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
                              {1, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * 4},
                          });
  pipeline.AddAttachment();
  // Enable depth bound test
  pipeline.DepthStencilState().depthBoundsTestEnable = VK_TRUE;
  pipeline.Commit();
  return pipeline;
}

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  {
    const float min_depth_bound = 0.01f;
    const float max_depth_bound = 0.99f;
    VkPhysicalDeviceFeatures request_features = {0};
    request_features.depthBounds = VK_TRUE;

    vulkan::VulkanApplication app(data->allocator(), data->logger(), data, {},
                                  {}, request_features);
    if (app.device().is_valid()) {
      auto pipeline = CreatePipelineWithDepthBoundTestEnabled(data, &app);
      auto cmdBuf = app.GetCommandBuffer();
      app.BeginCommandBuffer(&cmdBuf);
      cmdBuf->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline);
      cmdBuf->vkCmdSetDepthBounds(cmdBuf, min_depth_bound, max_depth_bound);
      app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&cmdBuf,
                                                       &app.render_queue());
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
