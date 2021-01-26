// Copyright 2020 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <utility>

#include "support/containers/unique_ptr.h"
#include "support/containers/unordered_map.h"
#include "support/containers/vector.h"
#include "support/entry/entry.h"
#include "vulkan_core.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_model.h"
#include "vulkan_wrapper/command_buffer_wrapper.h"
#include "vulkan_wrapper/descriptor_set_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

struct FrameData {
    containers::unique_ptr<vulkan::VkCommandBuffer> gCommandBuffer;
    containers::unique_ptr<vulkan::VkCommandBuffer> postCommandBuffer;

    containers::unique_ptr<vulkan::VkSemaphore> gRenderFinished;
    containers::unique_ptr<vulkan::VkSemaphore> imageAcquired;
    containers::unique_ptr<vulkan::VkSemaphore> postRenderFinished;

    containers::unique_ptr<vulkan::VkFence> renderingFence;
    containers::unique_ptr<vulkan::VkFence> postProcessFence;

    containers::unique_ptr<vulkan::DescriptorSet> descriptorSet;
};

namespace gBuffer {
uint32_t vert[] =
#include "triangle.vert.spv"
    ;

uint32_t frag[] =
#include "triangle.frag.spv"
    ;
};

namespace postBuffer {
uint32_t vert[] =
#include "postprocessing.vert.spv"
    ;

uint32_t frag[] =
#include "postprocessing.frag.spv"
    ;
};

namespace screen_model {
#include "fullscreen_quad.obj.h"
}
const auto& screen_data = screen_model::model;


const size_t COLOR_BUFFER_SIZE = sizeof(float) * 3;


const float colors[3][3] = {
    {1.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f},
};

vulkan::VkRenderPass buildRenderPass(
    vulkan::VulkanApplication *app,
    VkImageLayout initial_layout,
    VkImageLayout final_layout) {

    // Build render pass
    VkAttachmentReference color_attachment = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    vulkan::VkRenderPass render_pass = app->CreateRenderPass(
        {{
           0,                                // flags
           app->swapchain().format(),        // format
           VK_SAMPLE_COUNT_1_BIT,            // samples
           VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
           VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stenilLoadOp
           VK_ATTACHMENT_STORE_OP_DONT_CARE, // stenilStoreOp
           initial_layout,                   // initialLayout
           final_layout                      // finalLayout
        }},                                  // AttachmentDescriptions
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
        }},                                  // SubpassDescriptions
        {{
           VK_SUBPASS_EXTERNAL,                             // srcSubpass
           0,                                               // dstSubpass
           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,   // srcStageMask
           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,   // dstStageMsdk
           0,                                               // srcAccessMask
           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,            // dstAccessMask
           0                                                // dependencyFlags
        }}
    );

    return render_pass;
}

vulkan::VulkanGraphicsPipeline buildTrianglePipeline(
    vulkan::VulkanApplication *app,
    vulkan::VkRenderPass* render_pass) {
    // Build Triangle Pipeline
    vulkan::PipelineLayout pipeline_layout(app->CreatePipelineLayout({{}}));
    vulkan::VulkanGraphicsPipeline pipeline = app->CreateGraphicsPipeline(&pipeline_layout, render_pass, 0);

    vulkan::VulkanGraphicsPipeline::InputStream input_stream{
        0,                           // binding
        VK_FORMAT_R32G32B32_SFLOAT,  // format
        0                            // offset
    };
    
    pipeline.AddInputStream(COLOR_BUFFER_SIZE,VK_VERTEX_INPUT_RATE_VERTEX, {input_stream});
    pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", gBuffer::vert);
    pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", gBuffer::frag);

    pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline.SetScissor({
        {0, 0},                                                 // offset
        {app->swapchain().width(), app->swapchain().height()}   // extent
    });
    pipeline.SetViewport({
        0.0f,                                           // x
        0.0f,                                           // y
        static_cast<float>(app->swapchain().width()),   // width
        static_cast<float>(app->swapchain().height()),  // height
        0.0f,                                           // minDepth
        1.0f                                            // maxDepth
    });
    pipeline.SetSamples(VK_SAMPLE_COUNT_1_BIT);
    pipeline.AddAttachment();
    pipeline.Commit();

    return pipeline;
}

vulkan::VulkanGraphicsPipeline buildPostPipeline(
    vulkan::VulkanApplication *app,
    vulkan::PipelineLayout* pipeline_layout,
    vulkan::VkRenderPass* render_pass,
    vulkan::VulkanModel* screen) {
    // Build Post Pipeline
    vulkan::VulkanGraphicsPipeline pipeline = app->CreateGraphicsPipeline(pipeline_layout, render_pass, 0);

    pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", postBuffer::vert);
    pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", postBuffer::frag);

    pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline.SetInputStreams(screen);
    pipeline.SetScissor({
        {0, 0},                                                 // offset
        {app->swapchain().width(), app->swapchain().height()}   // extent
    });
    pipeline.SetViewport({
        0.0f,                                           // x
        0.0f,                                           // y
        static_cast<float>(app->swapchain().width()),   // width
        static_cast<float>(app->swapchain().height()),  // height
        0.0f,                                           // minDepth
        1.0f                                            // maxDepth
    });
    pipeline.SetSamples(VK_SAMPLE_COUNT_1_BIT);
    pipeline.AddAttachment();
    pipeline.Commit();

    return pipeline;
}

containers::vector<vulkan::ImagePointer> buildSamplerImages(
    vulkan::VulkanApplication *app,
    const entry::EntryData* data) {
    containers::vector<vulkan::ImagePointer> images(data->allocator());
    images.reserve(app->swapchain_images().size());

    for(int index=0; index<app->swapchain_images().size(); index++) {
        VkImageCreateInfo image_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,    // sType
            nullptr,                                // pNext
            0,                                      // flags
            VK_IMAGE_TYPE_2D,                       // imageType
            app->swapchain().format(),              // format
            {
                app->swapchain().width(),           // width
                app->swapchain().height(),          // height
                1                                   // depth
            },                                      // extent
            1,                                      // mipLevels
            1,                                      // arrayLayers
            VK_SAMPLE_COUNT_1_BIT,                  // samples
            VK_IMAGE_TILING_OPTIMAL,                // tiling
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,    // usage
            VK_SHARING_MODE_EXCLUSIVE,              // sharingMode
            0,                                      // queueFamilyIndexCount
            nullptr,                                // pQueueFamilyIndices
            VK_IMAGE_LAYOUT_UNDEFINED               // initialLayout
        };
        images.push_back(app->CreateAndBindImage(&image_create_info));
    }

    return images;
}

containers::vector<vulkan::VkImageView> buildSwapchainImageViews(
    vulkan::VulkanApplication *app,
    const entry::EntryData* data) {
    containers::vector<vulkan::VkImageView> image_views(data->allocator());
    image_views.reserve(app->swapchain_images().size());

    for(int index=0; index < app->swapchain_images().size(); index++) {
        VkImage& swapchain_image = app->swapchain_images()[index];

        VkImageViewCreateInfo image_view_create_info {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            swapchain_image,                           // image
            VK_IMAGE_VIEW_TYPE_2D,                     // viewType
            app->swapchain().format(),                 // format
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
        LOG_ASSERT(==, data->logger(), app->device()->vkCreateImageView(
            app->device(),
            &image_view_create_info,
            nullptr,
            &raw_image_view
        ), VK_SUCCESS);
        image_views.push_back(vulkan::VkImageView(raw_image_view, nullptr, &app->device()));
    }

    return image_views;
}

vulkan::DescriptorSet buildDescriptorSet(
    vulkan::VulkanApplication *app,
    const vulkan::VkSampler& sampler,
    const vulkan::VkImageView& image_view) {
    auto descriptor_set = app->AllocateDescriptorSet({{
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
    }});

    VkDescriptorImageInfo image_info = {
        sampler.get_raw_object(),
        image_view.get_raw_object(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
        nullptr,                                   // pNext
        descriptor_set,                            // dstSet
        0,                                         // dstbinding
        0,                                         // dstArrayElement
        1,                                         // descriptorCount
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
        &image_info,                               // pImageInfo
        nullptr,                                   // pBufferInfo
        nullptr,                                   // pTexelBufferView
    };

    app->device()->vkUpdateDescriptorSets(app->device(), 1, &write, 0, nullptr);

    return descriptor_set;
}

containers::vector<vulkan::VkImageView> buildSamplerImageViews(
    vulkan::VulkanApplication *app,
    const containers::vector<vulkan::ImagePointer>& images,
    const entry::EntryData* data) {
    containers::vector<vulkan::VkImageView> image_views(data->allocator());
    image_views.reserve(app->swapchain_images().size());

    for(int index=0; index < images.size(); index++) {
        auto sampler_image = images[index].get();

        VkImageViewCreateInfo image_view_create_info {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            sampler_image->get_raw_image(),            // image
            VK_IMAGE_VIEW_TYPE_2D,                     // viewType
            app->swapchain().format(),                 // format
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
        LOG_ASSERT(==, data->logger(), app->device()->vkCreateImageView(
            app->device(),
            &image_view_create_info,
            nullptr,
            &raw_image_view
        ), VK_SUCCESS);
        image_views.push_back(vulkan::VkImageView(raw_image_view, nullptr, &app->device()));
    }

    return image_views;
}

containers::vector<vulkan::VkFramebuffer> buildFramebuffers(
    vulkan::VulkanApplication *app,
    const vulkan::VkRenderPass& render_pass,
    const containers::vector<vulkan::VkImageView>& image_views,
    const entry::EntryData* data) {
    containers::vector<vulkan::VkFramebuffer> framebuffers(data->allocator());
    framebuffers.reserve(app->swapchain_images().size());

    for(int index=0; index < app->swapchain_images().size(); index++) {
        VkFramebufferCreateInfo framebuffer_create_info {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
            nullptr,                                    // pNext
            0,                                          // flags
            render_pass,                                // renderPass
            1,                                          // attachmentCount
            &image_views[index].get_raw_object(),       // attachments
            app->swapchain().width(),                   // width
            app->swapchain().height(),                  // height
            1                                           // layers
        };

        VkFramebuffer raw_framebuffer;
        LOG_ASSERT(==, data->logger(), app->device()->vkCreateFramebuffer(
            app->device(),
            &framebuffer_create_info,
            nullptr,
            &raw_framebuffer
        ), VK_SUCCESS);

        framebuffers.push_back(vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app->device()));
    }

    return framebuffers;
}

containers::unique_ptr<vulkan::VulkanApplication::Buffer> buildColorBuffer(vulkan::VulkanApplication *app) {
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        COLOR_BUFFER_SIZE,                     // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr
    };

    return app->CreateAndBindDeviceBuffer(&create_info);
}

int main_entry(const entry::EntryData* data) {
    data->logger()->LogInfo("Application Startup");

    // Enforce min_swapchain_image_count = 3
    vulkan::VulkanApplication app(
        data->allocator(), data->logger(), data, {}, {}, {0}, 
        10 * 1024 * 1024,
        512 * 1024 * 1024, 
        10 * 1024 * 1024, 
        1 * 1024 * 1024, false, false, false, 0, false,
        false, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, false, false, nullptr, false,
        false, nullptr, 3);

    vulkan::VulkanModel screen(data->allocator(), data->logger(), screen_data);

    // Initialize Screen Model
    auto init_cmd_buf = app.GetCommandBuffer();
    app.BeginCommandBuffer(&init_cmd_buf);

    screen.InitializeData(&app, &init_cmd_buf);
    vulkan::VkFence init_fence = CreateFence(&app.device());
    app.EndAndSubmitCommandBuffer(
        &init_cmd_buf,
        &app.render_queue(),
        {},
        {},
        {},
        init_fence.get_raw_object()
    );

    // Default Sampler
    auto sampler = CreateDefaultSampler(&app.device());

    // Geometry Render Pass
    auto g_render_pass = buildRenderPass(
        &app,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    auto g_pipeline = buildTrianglePipeline(&app, &g_render_pass);
    
    auto render_images = buildSamplerImages(&app, data);
    auto render_image_views = buildSamplerImageViews(&app, render_images, data);
    auto render_framebuffers = buildFramebuffers(&app, g_render_pass, render_image_views, data);

    // Post Render Pass
    auto post_render_pass = buildRenderPass(
        &app,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    auto post_pipeline_layout = app.CreatePipelineLayout({{{
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
    }}});
    auto post_pipeline = buildPostPipeline(&app, &post_pipeline_layout, &post_render_pass, &screen);
    auto post_image_views = buildSwapchainImageViews(&app, data);
    auto post_framebuffers = buildFramebuffers(
        &app,
        post_render_pass,
        post_image_views,
        data
    );

    auto colorBuffer = buildColorBuffer(&app);

    // FrameData
    containers::vector<FrameData> frameData(data->allocator());
    frameData.resize(app.swapchain_images().size());

    for(int index = 0; index < app.swapchain_images().size(); index++) {
        frameData[index].gCommandBuffer = containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(),
            app.GetCommandBuffer()
        );
        frameData[index].postCommandBuffer = containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(),
            app.GetCommandBuffer()
        );
        frameData[index].gRenderFinished = containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(),
            vulkan::CreateSemaphore(&app.device())
        );
        frameData[index].imageAcquired = containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(),
            vulkan::CreateSemaphore(&app.device())
        );
        frameData[index].postRenderFinished = containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(),
            vulkan::CreateSemaphore(&app.device())
        );
        frameData[index].renderingFence = containers::make_unique<vulkan::VkFence>(
            data->allocator(),
            vulkan::CreateFence(&app.device(), true)
        );
        frameData[index].postProcessFence = containers::make_unique<vulkan::VkFence>(
            data->allocator(),
            vulkan::CreateFence(&app.device(), true)
        );
        frameData[index].descriptorSet = containers::make_unique<vulkan::DescriptorSet>(
            data->allocator(),
            buildDescriptorSet(&app, sampler, render_image_views[index])
        );
    }

    uint32_t current_frame = 0;
    uint32_t image_index;

    VkClearValue clear_color = {0.40f, 0.94f, 0.59f, 1.0f};

    // MAIN LOOP ======

    while(!data->WindowClosing()) {
        app.device()->vkAcquireNextImageKHR(
            app.device(),
            app.swapchain().get_raw_object(),
            UINT64_MAX,
            frameData[current_frame].imageAcquired->get_raw_object(),
            static_cast<VkFence>(VK_NULL_HANDLE),
            &image_index
        );

        const VkFence renderingFence = frameData[current_frame].renderingFence->get_raw_object();
        const VkFence postProcessFence = frameData[current_frame].postProcessFence->get_raw_object();
        VkFence currentFrameFences[] = {renderingFence, postProcessFence};

        app.device()->vkWaitForFences(app.device(), 2, currentFrameFences, VK_FALSE, UINT64_MAX);

        if (app.device()->vkGetFenceStatus(app.device(), renderingFence) != VK_SUCCESS) {
            app.device()->vkWaitForFences(app.device(), 1, &renderingFence, VK_TRUE, UINT64_MAX);
        }

        app.device()->vkResetFences(app.device(), 1, &renderingFence);
  
        auto& geometry_buf = frameData[image_index].gCommandBuffer;
        auto& geo_ref = *geometry_buf;

        app.BeginCommandBuffer(geometry_buf.get());

        VkRenderPassBeginInfo g_pass_begin {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            g_render_pass,
            render_framebuffers[image_index].get_raw_object(),
            {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
            1,
            &clear_color
        };

        geo_ref->vkCmdBeginRenderPass(geo_ref, &g_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        geo_ref->vkCmdBindPipeline(geo_ref, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

        VkDeviceSize offsets[] = {0};
        VkBuffer vertexBuffers[1] = {*colorBuffer};
        geo_ref->vkCmdBindVertexBuffers(geo_ref, 0, 1, vertexBuffers, offsets);
        geo_ref->vkCmdDraw(geo_ref, 3, 1, 0, 0);
        geo_ref->vkCmdEndRenderPass(geo_ref);
        LOG_ASSERT(==, data->logger(), VK_SUCCESS,
            app.EndAndSubmitCommandBuffer(
                geometry_buf.get(),
                &app.render_queue(),
                {frameData[current_frame].imageAcquired->get_raw_object()},
                {
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                },
                {frameData[current_frame].gRenderFinished->get_raw_object()},
                renderingFence
            )
        );

        if (app.device()->vkGetFenceStatus(app.device(), postProcessFence) != VK_SUCCESS) {
            app.device()->vkWaitForFences(app.device(), 1, &postProcessFence, VK_TRUE, UINT64_MAX);
        }

        app.device()->vkResetFences(app.device(), 1, &postProcessFence);

        VkRenderPassBeginInfo post_pass_begin {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            post_render_pass,
            post_framebuffers[image_index].get_raw_object(),
            {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
            1,
            &clear_color
        };

        auto& post_cmd_buf = frameData[image_index].postCommandBuffer;
        vulkan::VkCommandBuffer& post_ref_cmd = *post_cmd_buf;

        app.BeginCommandBuffer(post_cmd_buf.get());
        vulkan::RecordImageLayoutTransition(
            app.swapchain_images()[image_index],
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            VK_IMAGE_LAYOUT_UNDEFINED,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            post_cmd_buf.get()
        );

        app.FillSmallBuffer(colorBuffer.get(), static_cast<const void*>(colors[current_frame % 3]),
        COLOR_BUFFER_SIZE, 0, &post_ref_cmd, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, 0);

        post_ref_cmd->vkCmdBeginRenderPass(post_ref_cmd, &post_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        post_ref_cmd->vkCmdBindDescriptorSets(
            post_ref_cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            post_pipeline_layout,
            0,
            1,
            &frameData[image_index].descriptorSet->raw_set(),
            0,
            nullptr
        );
        post_ref_cmd->vkCmdBindPipeline(post_ref_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post_pipeline);
        screen.Draw(&post_ref_cmd);
        post_ref_cmd->vkCmdEndRenderPass(post_ref_cmd);
        LOG_ASSERT(==, data->logger(), VK_SUCCESS,
            app.EndAndSubmitCommandBuffer(
                &post_ref_cmd,
                &app.render_queue(),
                {frameData[current_frame].gRenderFinished->get_raw_object()},
                {
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                },
                {frameData[current_frame].postRenderFinished->get_raw_object()}, // postPass is done
                postProcessFence // frame fence
            )
        );

        VkPresentInfoKHR present_info {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            nullptr,
            1,
            &frameData[current_frame].postRenderFinished->get_raw_object(),
            1,
            &app.swapchain().get_raw_object(),
            &image_index,
            nullptr
        };

        app.present_queue()->vkQueuePresentKHR(app.present_queue(), &present_info);

        current_frame = (current_frame + 1) % app.swapchain_images().size();
    }

    app.device()->vkDeviceWaitIdle(app.device());
    data->logger()->LogInfo("Application Shutdown");

    return 0;
}