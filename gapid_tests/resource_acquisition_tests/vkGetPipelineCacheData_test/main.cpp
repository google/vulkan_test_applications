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
#include "simple_fragment.frag.spv"
    ;

uint32_t vertex_shader[] =
#include "simple_vertex.vert.spv"
    ;

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  {
    // 1. Create an empty pipeline cache, then get its data.
    vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
    vulkan::VkInstance instance(
        vulkan::CreateDefaultInstance(data->root_allocator, &wrapper));
    vulkan::VkDevice device(
        vulkan::CreateDefaultDevice(data->root_allocator, instance, false));

    VkPipelineCacheCreateInfo create_info{
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,  // sType
        nullptr,                                       // pNext
        0,                                             // flags
        0,                                             // initialDataSize
        nullptr                                        // pInitialData
    };

    VkPipelineCache cache;
    LOG_ASSERT(==, data->log, device->vkCreatePipelineCache(
                                  device, &create_info, nullptr, &cache),
               VK_SUCCESS);

    // pData is null
    size_t cache_data_size = 0;
    LOG_ASSERT(==, data->log, device->vkGetPipelineCacheData(
                                  device, cache, &cache_data_size, nullptr),
               VK_SUCCESS);

    // pData is not null and pDataSize refer to the size of the cache data
    containers::vector<uint8_t> cache_data(cache_data_size, 0,
                                           data->root_allocator);
    LOG_ASSERT(==, data->log,
               device->vkGetPipelineCacheData(device, cache, &cache_data_size,
                                              cache_data.data()),
               VK_SUCCESS);

    // Check the content of the pipeline cache data header
    size_t cache_data_header_size =
        size_t(cache_data[0]) | (size_t(cache_data[1]) << 8) |
        (size_t(cache_data[2]) << 16) | (size_t(cache_data[3]) << 24);
    LOG_ASSERT(==, data->log, cache_data_header_size, 16 + VK_UUID_SIZE);
    LOG_ASSERT(==, data->log, cache_data[4],
               uint8_t(VK_PIPELINE_CACHE_HEADER_VERSION_ONE));

    device->vkDestroyPipelineCache(device, cache, nullptr);
  }

  {
    // 2. Create a graphics pipeline with pipeline cache, then get its data
    // Create pipeline layout
    vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data);
    vulkan::VkDevice& dev = app.device();
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
    // Create renderpass
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
         }},  // AttachmentDescriptions
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

    // Create shader modules
    vulkan::VkShaderModule vertex_shader_module =
        app.CreateShaderModule(vertex_shader);
    vulkan::VkShaderModule fragment_shader_module =
        app.CreateShaderModule(fragment_shader);

    // Create graphics pipeline
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

    VkVertexInputBindingDescription vertex_binding_description = {
        0,                           // binding
        4 * 6, /* vec4 + vec2 */     // stride
        VK_VERTEX_INPUT_RATE_VERTEX  // inputRate
    };

    VkVertexInputAttributeDescription vertex_attribute_descriptions[2] = {
        {
            0,                              // location
            0,                              // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,  // format
            0                               // offset
        },
        {
            1,                        // location
            0,                        // binding
            VK_FORMAT_R32G32_SFLOAT,  // format
            4 * 4                     // offset
        }};

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        1,                             // vertexBindingDescriptionCount
        &vertex_binding_description,   // pVertexBindingDescriptions
        2,                             // vertexAttributeDescriptionCount
        vertex_attribute_descriptions  // pVertexAttributeDescriptions
    };

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

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,  // sType
        nullptr,                                                     // pNext
        0,                                                           // flags
        VK_TRUE,             // depthTestEnable
        VK_TRUE,             // depthWriteEnable
        VK_COMPARE_OP_LESS,  // depthCompareOp
        VK_FALSE,            // depthBoundsTestEnable
        VK_FALSE,            // stencilTestEnable
        {
            VK_STENCIL_OP_KEEP,   // front.failOp
            VK_STENCIL_OP_KEEP,   // front.passOp
            VK_STENCIL_OP_KEEP,   // front.depthFailOp
            VK_COMPARE_OP_NEVER,  // front.compareOp
            0x0,                  // front.compareMask
            0x0,                  // front.writeMask
            0                     // front.reference
        },
        {
            VK_STENCIL_OP_KEEP,   // back.failOp
            VK_STENCIL_OP_KEEP,   // back.passOp
            VK_STENCIL_OP_KEEP,   // back.depthFailOp
            VK_COMPARE_OP_NEVER,  // back.compareOp
            0x0,                  // back.compareMask
            0x0,                  // back.writeMask
            0                     // back.reference
        },
        0.0f,  // minDepthBounds
        1.0f   // maxDepthBounds
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        VK_FALSE,              // blendEnable
        VK_BLEND_FACTOR_ZERO,  // srcColorBlendFactor
        VK_BLEND_FACTOR_ONE,   // dstColorBlendFactor
        VK_BLEND_OP_ADD,       // colorBlendOp
        VK_BLEND_FACTOR_ZERO,  // srcAlphaBlendFactor
        VK_BLEND_FACTOR_ONE,   // dstAlphaBlendFactor
        VK_BLEND_OP_ADD,       // alphaBlendOp
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT,  // colorWriteMask
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
        &depth_stencil_state,                             // pDepthStencilState
        &color_blend_state,                               // pColorBlendState
        nullptr,                                          // pDynamicState
        pipeline_layout,                                  // layout
        render_pass,                                      // renderPass
        0,                                                // subpass
        VK_NULL_HANDLE,                                   // basePipelineHandle
        0                                                 // basePipelineIndex
    };

    VkPipeline raw_pipeline;
    LOG_ASSERT(
        ==, data->log, VK_SUCCESS,
        dev->vkCreateGraphicsPipelines(dev, app.pipeline_cache(), 1,
                                       &create_info, nullptr, &raw_pipeline));
    dev->vkDestroyPipeline(dev, raw_pipeline, nullptr);

    // Get the pipeline cache data
    size_t cache_data_size = 0;
    LOG_ASSERT(==, data->log,
               dev->vkGetPipelineCacheData(dev, app.pipeline_cache(),
                                           &cache_data_size, nullptr),
               VK_SUCCESS);

    containers::vector<uint8_t> cache_data(cache_data_size, 0,
                                           data->root_allocator);
    LOG_ASSERT(==, data->log,
               dev->vkGetPipelineCacheData(dev, app.pipeline_cache(),
                                           &cache_data_size, cache_data.data()),
               VK_SUCCESS);

    // Check the content of the pipeline cache data header
    size_t cache_data_header_size =
        size_t(cache_data[0]) | (size_t(cache_data[1]) << 8) |
        (size_t(cache_data[2]) << 16) | (size_t(cache_data[3]) << 24);
    LOG_ASSERT(==, data->log, cache_data_header_size, 16 + VK_UUID_SIZE);
    LOG_ASSERT(==, data->log, cache_data[4],
               uint8_t(VK_PIPELINE_CACHE_HEADER_VERSION_ONE));
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
