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

uint32_t fragment_shader[] =
#include "hardcode_pos_triangle.frag.spv"
    ;

uint32_t vertex_shader[] =
#include "hardcode_pos_triangle.vert.spv"
    ;

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data);
  vulkan::VkDevice& device = app.device();

  {
    // Create render pass.

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    vulkan::VkRenderPass render_pass = app.CreateRenderPass(
        {{
            0,                                         // flags
            app.swapchain().format(),                  // format
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
            nullptr,                          // pDepthStencilAttachment
            0,                                // preserveAttachmentCount
            nullptr                           // pPreserveAttachments
        }},                                   // SubpassDescriptions
        {}                                    // SubpassDependencies
        );

    // Create shader modules.

    vulkan::VkShaderModule vertex_shader_module =
        app.CreateShaderModule(vertex_shader);
    vulkan::VkShaderModule fragment_shader_module =
        app.CreateShaderModule(fragment_shader);
    VkPipelineShaderStageCreateInfo shader_stage_create_infos[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
            nullptr,                                              // pNext
            0,                                                    // flags
            VK_SHADER_STAGE_VERTEX_BIT,                           // stage
            vertex_shader_module,                                 // module
            "main",                                               // pName
            nullptr  // pSpecializationInfo
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
            nullptr,                                              // pNext
            0,                                                    // flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                         // stage
            fragment_shader_module,                               // module
            "main",                                               // pName
            nullptr  // pSpecializationInfo
        }};

    // Specify vertex input state.

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        0,        // vertexBindingDescriptionCount
        nullptr,  // pVertexBindingDescriptions
        0,        // vertexAttributeDescriptionCount
        nullptr,  // pVertexAttributeDescriptions
    };

    // Other fixed function stage configuration.

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // sType
        nullptr,                                                      // pNext
        0,                                                            // flags
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // topology
        VK_FALSE                              // primitiveRestartEnable
    };

    VkViewport viewport = {
        0.0f,                                          // x
        0.0f,                                          // y
        static_cast<float>(app.swapchain().width()),   // width
        static_cast<float>(app.swapchain().height()),  // height
        0.0f,                                          // minDepth
        1.0f,                                          // maxDepth
    };

    VkRect2D scissor = {{
                            0,  // offset.x
                            0,  // offset.y
                        },
                        {
                            app.swapchain().width(),  // extent.width
                            app.swapchain().height()  // extent.height
                        }};

    VkPipelineViewportStateCreateInfo viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  // sType
        nullptr,                                                // pNext
        0,                                                      // flags
        1,                                                      // viewportCount
        &viewport,                                              // pViewports
        1,                                                      // scissorCount
        &scissor,                                               // pScissors
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,  // sType
        nullptr,                                                     // pNext
        0,                                                           // flags
        VK_FALSE,                 // depthClampEnable
        VK_FALSE,                 // rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,     // polygonMode
        VK_CULL_MODE_BACK_BIT,    // cullMode
        VK_FRONT_FACE_CLOCKWISE,  // frontFace
        VK_FALSE,                 // depthBiasEnable
        0.0f,                     // depthBiasConstantFactor
        0.0f,                     // depthBiasClamp
        0.0f,                     // depthBiasSlopeFactor
        1.0f,                     // lineWidth
    };

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,  // sType
        nullptr,                                                   // pNext
        0,                                                         // flags
        VK_SAMPLE_COUNT_1_BIT,  // rasterizationSamples
        VK_FALSE,               // sampleShadingEnable
        0,                      // minSampleShading
        0,                      // pSampleMask
        VK_FALSE,               // alphaToCoverageEnable
        VK_FALSE,               // alphaToOneEnable
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        VK_FALSE,              // blendEnable
        VK_BLEND_FACTOR_ZERO,  // srcColorBlendFactor
        VK_BLEND_FACTOR_ONE,   // dstColorBlendFactor
        VK_BLEND_OP_ADD,       // colorBlendOp
        VK_BLEND_FACTOR_ZERO,  // srcAlphaBlendFactor
        VK_BLEND_FACTOR_ONE,   // dstAlphaBlendFactor
        VK_BLEND_OP_ADD,       // alphaBlendOp
        (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
         VK_COLOR_COMPONENT_B_BIT |
         VK_COLOR_COMPONENT_A_BIT),  // colorWriteMask
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,  // sType
        nullptr,                                                   // pNext
        0,                                                         // flags
        VK_FALSE,                       // logicOpEnable
        VK_LOGIC_OP_CLEAR,              // logicOp
        1,                              // attachmentCount
        &color_blend_attachment_state,  // pAttachments
        {1.0f, 1.0f, 1.0f, 1.0f}        // blendConstants
    };

    vulkan::PipelineLayout pipeline_layout(app.CreatePipelineLayout({{}}));

    // Create the graphics pipeline.

    VkGraphicsPipelineCreateInfo create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,  // sType
        nullptr,                                          // pNext
        0,                                                // flags
        2,                                                // stageCount
        shader_stage_create_infos,                        // pStages
        &vertex_input_state,                              // pVertexInputState
        &input_assembly_state,                            // pInputAssemblyState
        nullptr,                                          // pTessellationState
        &viewport_state,                                  // pViewportState
        &rasterization_state,                             // pRasterizationState
        &multisample_state,                               // pMultisampleState
        nullptr,                                          // pDepthStencilState
        &color_blend_state,                               // pColorBlendState
        nullptr,                                          // pDynamicState
        pipeline_layout,                                  // layout
        render_pass,                                      // renderPass
        0,                                                // subpass
        VK_NULL_HANDLE,                                   // basePipelineHandle
        0                                                 // basePipelineIndex
    };

    VkPipeline raw_pipeline;
    LOG_EXPECT(==, data->log, device->vkCreateGraphicsPipelines(
                                  device, app.pipeline_cache(), 1, &create_info,
                                  nullptr, &raw_pipeline),
               VK_SUCCESS);
    vulkan::VkPipeline pipeline(raw_pipeline, nullptr, &device);

    // Create image view.

    VkImageViewCreateInfo image_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        app.swapchain_images().front(),            // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        app.swapchain().format(),                  // format
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
    LOG_EXPECT(==, data->log, app.device()->vkCreateImageView(
                                  app.device(), &image_view_create_info,
                                  nullptr, &raw_image_view),
               VK_SUCCESS);
    vulkan::VkImageView image_view(raw_image_view, nullptr, &app.device());

    // Create framebuffer

    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        render_pass,                                // renderPass
        1,                                          // attachmentCount
        &raw_image_view,                            // attachments
        app.swapchain().width(),                    // width
        app.swapchain().height(),                   // height
        1                                           // layers
    };
    ::VkFramebuffer raw_framebuffer;
    app.device()->vkCreateFramebuffer(app.device(), &framebuffer_create_info,
                                      nullptr, &raw_framebuffer);
    vulkan::VkFramebuffer framebuffer(raw_framebuffer, nullptr, &app.device());

    // Create query pool.
    vulkan::VkQueryPool query_pool = vulkan::CreateQueryPool(
        &app.device(), {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0,
                        VK_QUERY_TYPE_OCCLUSION, 2, 0});

    // Create command buffer.
    vulkan::VkCommandBuffer command_buffer = app.GetCommandBuffer();
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
            {0,                          // renderArea.offset.x
             0},                         // renderArea.offset.y
            {app.swapchain().width(),    // renderArea.extent.width
             app.swapchain().height()},  // renderArea.extent.height
        },
        0,        // clearValueCount
        nullptr,  // pClearValues
    };

    command_buffer->vkCmdResetQueryPool(command_buffer, query_pool, 0, 2);
    // Begin a query with precise bit set
    command_buffer->vkCmdBeginQuery(command_buffer, query_pool, 0,
                                    VK_QUERY_CONTROL_PRECISE_BIT);
    // Begin a query with precise bit not set
    command_buffer->vkCmdBeginQuery(command_buffer, query_pool, 1, 0);

    command_buffer->vkCmdBeginRenderPass(
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->vkCmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raw_pipeline);
    command_buffer->vkCmdDraw(command_buffer, 3, 1, 0, 0);
    command_buffer->vkCmdEndRenderPass(command_buffer);

    // End the two queries
    command_buffer->vkCmdEndQuery(command_buffer, query_pool, 0);
    command_buffer->vkCmdEndQuery(command_buffer, query_pool, 1);

    command_buffer->vkEndCommandBuffer(command_buffer);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
