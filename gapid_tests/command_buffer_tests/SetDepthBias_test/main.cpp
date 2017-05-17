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

void CreatePipelineAndSetDepthBias(const entry::entry_data* data,
                                   vulkan::VulkanApplication* app_ptr,
                                   float depth_bias_constant_factor,
                                   float depth_bias_clamp,
                                   float depth_bias_slope_factor) {
  LOG_ASSERT(!=, data->log, 0, (uintptr_t)app_ptr);
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
  pipeline.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);
  pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline.AddInputStream(4 * 6,  // vec4 + vec2
                          VK_VERTEX_INPUT_RATE_VERTEX,
                          {
                              {0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
                              {1, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * 4},
                          });
  pipeline.AddAttachment();
  pipeline.Commit();

  vulkan::VkCommandBuffer command_buffer = app.GetCommandBuffer();
  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
      nullptr,                                      // pNext
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // flags
      nullptr                                       // pInheritanceInfo
  };

  command_buffer->vkBeginCommandBuffer(command_buffer, &begin_info);
  command_buffer->vkCmdBindPipeline(command_buffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  command_buffer->vkCmdSetDepthBias(command_buffer, depth_bias_constant_factor,
                                    depth_bias_clamp, depth_bias_slope_factor);

  command_buffer->vkEndCommandBuffer(command_buffer);
}

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  {
    const float depth_bias_constant_factor = 1.1;
    const float depth_bias_clamp = 0.0;  // only 0.0 is valid as depthBiasClamp
                                         // has not been confirmed as supported.
    const float depth_bias_slope_factor = 3.3;
    // 1. Test with depthBiasClamp set to zero, so physical device feature:
    // depthBiasClamp is not required.
    vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data);
    CreatePipelineAndSetDepthBias(data, &app, depth_bias_constant_factor,
                                  depth_bias_clamp, depth_bias_slope_factor);
  }

  {
    // 2. Test with depthBiasClamp not set to zero, i.e. depthBiasClamp is
    // required.
    const float depth_bias_constant_factor = 1.1;
    const float depth_bias_clamp = 2.2;
    const float depth_bias_slope_factor = 3.3;
    VkPhysicalDeviceFeatures request_features = {0};
    request_features.depthBiasClamp = VK_TRUE;
    vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data,
                                  {}, request_features);
    if (app.device().is_valid()) {
      CreatePipelineAndSetDepthBias(data, &app, depth_bias_constant_factor,
                                    depth_bias_clamp, depth_bias_slope_factor);
    } else {
      data->log->LogInfo(
          "Disabled test due to missing physical device feature: "
          "depthBiasClamp");
    }
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
