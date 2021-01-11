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

#include <chrono>
#include <memory>
#include <utility>

#include "support/containers/unique_ptr.h"
#include "support/containers/unordered_map.h"
#include "support/containers/vector.h"
#include "support/entry/entry.h"
#include "vulkan_core.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"
#include "vulkan_wrapper/command_buffer_wrapper.h"
#include "vulkan_wrapper/descriptor_set_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

// This application implements overlapping frames: at each main loop iteration,
// several frames are being prepared. This technique is employed by some engines
// to pipeline frame creation.
//
// Here, each frame is rendered with two renderpasses: first, the "gbuffer"
// renderpass renders a triangle, then the "postprocessing" renderpass inverts
// the framebuffer colors. At each main iteration loop, the gbuffer renderpass
// of frame N+1 and the postprocessing renderpass of frame N are run, such that
// if we unroll the queue submissions we obtain:
//
// - ...
// - gbuffer frame N+1
// - postprocessing + present frame N
// - gbuffer frame N+2
// - postprocessing + present frame N+1
// - gbuffer frame N+3
// - postprocessing + present frame N+2
// - ...
//
// This effectively interleaves queue submissions of work for different frames,
// thus leading to overlapping frame preparation.

struct FrameData {
  // Command Buffers
  containers::unique_ptr<vulkan::VkCommandBuffer> gCommandBuffer;
  containers::unique_ptr<vulkan::VkCommandBuffer> postCommandBuffer;

  // Semaphores
  containers::unique_ptr<vulkan::VkSemaphore> gRenderFinished;
  containers::unique_ptr<vulkan::VkSemaphore> imageAcquired;
  containers::unique_ptr<vulkan::VkSemaphore> postRenderFinished;

  // Fences
  containers::unique_ptr<vulkan::VkFence> renderingFence;

  // Descriptor Sets
  containers::unique_ptr<vulkan::DescriptorSet> descriptorSet;
};

struct GeometryPushConstantData {
  float time;
};

namespace gBuffer {
uint32_t vert[] =
#include "triangle.vert.spv"
    ;

uint32_t frag[] =
#include "triangle.frag.spv"
    ;
};  // namespace gBuffer

namespace postBuffer {
uint32_t vert[] =
#include "postprocessing.vert.spv"
    ;

uint32_t frag[] =
#include "postprocessing.frag.spv"
    ;
};  // namespace postBuffer

namespace screenModel {
#include "fullscreen_quad.obj.h"
}
const auto& screen_data = screenModel::model;

vulkan::DescriptorSet buildDescriptorSet(
    vulkan::VulkanApplication* app, const vulkan::VkSampler& sampler,
    const vulkan::VkImageView& image_view) {
  auto descriptor_set =
      app->AllocateDescriptorSet({{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}});

  VkDescriptorImageInfo image_info = {sampler.get_raw_object(),
                                      image_view.get_raw_object(),
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  VkWriteDescriptorSet write{
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,     // sType
      nullptr,                                    // pNext
      descriptor_set,                             // dstSet
      0,                                          // dstbinding
      0,                                          // dstArrayElement
      1,                                          // descriptorCount
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // descriptorType
      &image_info,                                // pImageInfo
      nullptr,                                    // pBufferInfo
      nullptr,                                    // pTexelBufferView
  };

  app->device()->vkUpdateDescriptorSets(app->device(), 1, &write, 0, nullptr);

  return descriptor_set;
}

vulkan::VkRenderPass buildRenderPass(vulkan::VulkanApplication* app,
                                     VkImageLayout initial_layout,
                                     VkImageLayout final_layout) {
  // Build render pass
  VkAttachmentReference color_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  vulkan::VkRenderPass render_pass = app->CreateRenderPass(
      {{
          0,                                 // flags
          app->swapchain().format(),         // format
          VK_SAMPLE_COUNT_1_BIT,             // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
          initial_layout,                    // initialLayout
          final_layout                       // finalLayout
      }},                                    // AttachmentDescriptions
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
      {{
          VK_SUBPASS_EXTERNAL,                            // srcSubpass
          0,                                              // dstSubpass
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // dstStageMsdk
          0,                                              // srcAccessMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,  // dstAccessMask
          0                                          // dependencyFlags
      }});

  return render_pass;
}

vulkan::VulkanGraphicsPipeline buildTrianglePipeline(
    vulkan::VulkanApplication* app, vulkan::VkRenderPass* render_pass,
    vulkan::PipelineLayout* pipeline_layout) {
  // Build Triangle Pipeline
  vulkan::VulkanGraphicsPipeline pipeline =
      app->CreateGraphicsPipeline(pipeline_layout, render_pass, 0);

  pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", gBuffer::vert);
  pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", gBuffer::frag);

  pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline.SetScissor({
      {0, 0},                                                // offset
      {app->swapchain().width(), app->swapchain().height()}  // extent
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
    vulkan::VulkanApplication* app, vulkan::PipelineLayout* pipeline_layout,
    vulkan::VkRenderPass* render_pass, vulkan::VulkanModel* screen) {
  // Build Post Pipeline
  vulkan::VulkanGraphicsPipeline pipeline =
      app->CreateGraphicsPipeline(pipeline_layout, render_pass, 0);

  pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", postBuffer::vert);
  pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", postBuffer::frag);

  pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline.SetInputStreams(screen);
  pipeline.SetScissor({
      {0, 0},                                                // offset
      {app->swapchain().width(), app->swapchain().height()}  // extent
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
    vulkan::VulkanApplication* app, const entry::EntryData* data) {
  containers::vector<vulkan::ImagePointer> images(data->allocator());
  images.reserve(app->swapchain_images().size());

  for (int index = 0; index < app->swapchain_images().size(); index++) {
    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        app->swapchain().format(),            // format
        {
            app->swapchain().width(),   // width
            app->swapchain().height(),  // height
            1                           // depth
        },                              // extent
        1,                              // mipLevels
        1,                              // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,          // samples
        VK_IMAGE_TILING_OPTIMAL,        // tiling
        VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED                 // initialLayout
    };
    images.push_back(app->CreateAndBindImage(&image_create_info));
  }

  return images;
}

containers::vector<vulkan::VkImageView> buildSwapchainImageViews(
    vulkan::VulkanApplication* app, const entry::EntryData* data) {
  containers::vector<vulkan::VkImageView> image_views(data->allocator());
  image_views.reserve(app->swapchain_images().size());

  for (int index = 0; index < app->swapchain_images().size(); index++) {
    VkImage& swapchain_image = app->swapchain_images()[index];

    VkImageViewCreateInfo image_view_create_info{
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
    LOG_ASSERT(
        ==, data->logger(),
        app->device()->vkCreateImageView(app->device(), &image_view_create_info,
                                         nullptr, &raw_image_view),
        VK_SUCCESS);
    image_views.push_back(
        vulkan::VkImageView(raw_image_view, nullptr, &app->device()));
  }

  return image_views;
}

containers::vector<vulkan::VkImageView> buildSamplerImageViews(
    vulkan::VulkanApplication* app,
    const containers::vector<vulkan::ImagePointer>& images,
    const entry::EntryData* data) {
  containers::vector<vulkan::VkImageView> image_views(data->allocator());
  image_views.reserve(app->swapchain_images().size());

  for (int index = 0; index < images.size(); index++) {
    auto sampler_image = images[index].get();

    VkImageViewCreateInfo image_view_create_info{
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
    LOG_ASSERT(
        ==, data->logger(),
        app->device()->vkCreateImageView(app->device(), &image_view_create_info,
                                         nullptr, &raw_image_view),
        VK_SUCCESS);
    image_views.push_back(
        vulkan::VkImageView(raw_image_view, nullptr, &app->device()));
  }

  return image_views;
}

containers::vector<vulkan::VkFramebuffer> buildFramebuffers(
    vulkan::VulkanApplication* app, const vulkan::VkRenderPass& render_pass,
    const containers::vector<vulkan::VkImageView>& image_views,
    const entry::EntryData* data) {
  containers::vector<vulkan::VkFramebuffer> framebuffers(data->allocator());
  framebuffers.reserve(app->swapchain_images().size());

  for (int index = 0; index < app->swapchain_images().size(); index++) {
    VkFramebufferCreateInfo framebuffer_create_info{
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
    LOG_ASSERT(
        ==, data->logger(),
        app->device()->vkCreateFramebuffer(
            app->device(), &framebuffer_create_info, nullptr, &raw_framebuffer),
        VK_SUCCESS);

    framebuffers.push_back(
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app->device()));
  }

  return framebuffers;
}

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  // Enforce min_swapchain_image_count = 3
  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, {}, {},
                                {0}, 1024 * 1024, 1024 * 1024, 1024 * 1024,
                                1024 * 1024, false, false, false, 0, false,
                                false, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, false,
                                false, nullptr, false, false, nullptr, 3);

  auto sampler_images = buildSamplerImages(&app, data);
  vulkan::VulkanModel screen(data->allocator(), data->logger(), screen_data);

  // Initialize Screen Model
  auto init_cmd_buf = app.GetCommandBuffer();
  app.BeginCommandBuffer(&init_cmd_buf);

  screen.InitializeData(&app, &init_cmd_buf);
  vulkan::VkFence init_fence = CreateFence(&app.device());
  app.EndAndSubmitCommandBuffer(&init_cmd_buf, &app.render_queue(), {}, {}, {},
                                init_fence.get_raw_object());

  // Default Sampler
  auto sampler = CreateDefaultSampler(&app.device());

  // Geometry Render Pass
  auto g_render_pass =
      buildRenderPass(&app, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  VkPushConstantRange range;
  range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  range.offset = 0;
  range.size = (uint32_t)sizeof(GeometryPushConstantData);

  auto g_pipeline_layout = app.CreatePipelineLayout({{{}}}, {range});
  auto g_pipeline =
      buildTrianglePipeline(&app, &g_render_pass, &g_pipeline_layout);
  auto g_image_views = buildSamplerImageViews(&app, sampler_images, data);
  auto g_framebuffers =
      buildFramebuffers(&app, g_render_pass, g_image_views, data);

  // Post Render Pass
  auto post_render_pass = buildRenderPass(&app, VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  auto post_pipeline_layout =
      app.CreatePipelineLayout({{{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}}});
  auto post_pipeline = buildPostPipeline(&app, &post_pipeline_layout,
                                         &post_render_pass, &screen);
  auto post_image_views = buildSwapchainImageViews(&app, data);
  auto post_framebuffers =
      buildFramebuffers(&app, post_render_pass, post_image_views, data);

  // FrameData
  containers::vector<FrameData> frameData(data->allocator());
  frameData.resize(app.swapchain_images().size());

  for (int index = 0; index < app.swapchain_images().size(); index++) {
    frameData[index].gCommandBuffer =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());
    frameData[index].postCommandBuffer =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());
    frameData[index].gRenderFinished =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frameData[index].imageAcquired =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frameData[index].postRenderFinished =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frameData[index].renderingFence = containers::make_unique<vulkan::VkFence>(
        data->allocator(), vulkan::CreateFence(&app.device(), 1));
    frameData[index].descriptorSet =
        containers::make_unique<vulkan::DescriptorSet>(
            data->allocator(),
            buildDescriptorSet(&app, sampler, g_image_views[index]));
  }

  VkClearValue clear_color = {0.40f, 0.94f, 0.59f, 1.0f};

  uint32_t current_frame = 0;
  uint32_t next_frame = 1;
  uint32_t image_index;
  std::chrono::steady_clock::time_point start_time_point =
      std::chrono::steady_clock::now();
  const float triangle_speed = 0.01f;
  GeometryPushConstantData g_push_constant_data{0.0f};

  // First Geometry RP
  app.device()->vkResetFences(
      app.device(), 1,
      &frameData[current_frame].renderingFence->get_raw_object());

  VkRenderPassBeginInfo g_pass_begin{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      g_render_pass,
      g_framebuffers[current_frame].get_raw_object(),
      {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
      1,
      &clear_color};

  auto& cmd_buf = frameData[current_frame].gCommandBuffer;
  auto& ref_buf = *cmd_buf;

  app.BeginCommandBuffer(cmd_buf.get());

  ref_buf->vkCmdBeginRenderPass(ref_buf, &g_pass_begin,
                                VK_SUBPASS_CONTENTS_INLINE);
  ref_buf->vkCmdBindPipeline(ref_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             g_pipeline);
  ref_buf->vkCmdPushConstants(
      ref_buf, g_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
      sizeof(GeometryPushConstantData), &g_push_constant_data);
  ref_buf->vkCmdDraw(ref_buf, 3, 1, 0, 0);
  ref_buf->vkCmdEndRenderPass(ref_buf);
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             app.EndAndSubmitCommandBuffer(
                 cmd_buf.get(), &app.render_queue(), {}, {},
                 {frameData[current_frame].gRenderFinished->get_raw_object()},
                 VK_NULL_HANDLE));

  app.device()->vkWaitForFences(app.device(), 1, &init_fence.get_raw_object(),
                                VK_TRUE, UINT64_MAX);

  // MAIN LOOP ======

  while (!data->WindowClosing()) {
    // Geometry RP for next_frame
    app.device()->vkWaitForFences(
        app.device(), 1,
        &frameData[next_frame].renderingFence->get_raw_object(), VK_TRUE,
        UINT64_MAX);

    app.device()->vkResetFences(
        app.device(), 1,
        &frameData[next_frame].renderingFence->get_raw_object());

    VkRenderPassBeginInfo g_pass_begin{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        g_render_pass,
        g_framebuffers[next_frame].get_raw_object(),
        {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
        1,
        &clear_color};

    // Update Push Constants
    std::chrono::steady_clock::time_point current_time_point =
        std::chrono::steady_clock::now();
    float time_lapse = std::chrono::duration_cast<std::chrono::microseconds>(
                           current_time_point - start_time_point)
                           .count() /
                       1000.0;
    g_push_constant_data.time = triangle_speed * time_lapse;

    auto& geometry_buf = frameData[next_frame].gCommandBuffer;
    auto& geo_ref = *geometry_buf;

    app.BeginCommandBuffer(geometry_buf.get());

    geo_ref->vkCmdBeginRenderPass(geo_ref, &g_pass_begin,
                                  VK_SUBPASS_CONTENTS_INLINE);
    geo_ref->vkCmdBindPipeline(geo_ref, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               g_pipeline);
    geo_ref->vkCmdPushConstants(
        geo_ref, g_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
        sizeof(GeometryPushConstantData), &g_push_constant_data);
    geo_ref->vkCmdDraw(geo_ref, 3, 1, 0, 0);
    geo_ref->vkCmdEndRenderPass(geo_ref);
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               app.EndAndSubmitCommandBuffer(
                   geometry_buf.get(), &app.render_queue(), {}, {},
                   {frameData[next_frame].gRenderFinished->get_raw_object()},
                   VK_NULL_HANDLE));

    // Post Processing RP for current_frame

    app.device()->vkAcquireNextImageKHR(
        app.device(), app.swapchain().get_raw_object(), UINT64_MAX,
        frameData[current_frame].imageAcquired->get_raw_object(),
        static_cast<VkFence>(VK_NULL_HANDLE), &image_index);

    VkRenderPassBeginInfo post_pass_begin{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        post_render_pass,
        post_framebuffers[current_frame].get_raw_object(),
        {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
        1,
        &clear_color};

    auto& post_cmd_buf = frameData[current_frame].postCommandBuffer;
    vulkan::VkCommandBuffer& post_ref_cmd = *post_cmd_buf;

    app.BeginCommandBuffer(post_cmd_buf.get());
    vulkan::RecordImageLayoutTransition(
        app.swapchain_images()[image_index],
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, VK_IMAGE_LAYOUT_UNDEFINED, 0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, post_cmd_buf.get());

    post_ref_cmd->vkCmdBeginRenderPass(post_ref_cmd, &post_pass_begin,
                                       VK_SUBPASS_CONTENTS_INLINE);
    post_ref_cmd->vkCmdBindDescriptorSets(
        post_ref_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post_pipeline_layout, 0,
        1, &frameData[current_frame].descriptorSet->raw_set(), 0, nullptr);
    post_ref_cmd->vkCmdBindPipeline(
        post_ref_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post_pipeline);
    screen.Draw(&post_ref_cmd);
    post_ref_cmd->vkCmdEndRenderPass(post_ref_cmd);
    LOG_ASSERT(
        ==, data->logger(), VK_SUCCESS,
        app.EndAndSubmitCommandBuffer(
            &post_ref_cmd, &app.render_queue(),
            {
                frameData[current_frame]
                    .imageAcquired->get_raw_object(),  // wait for image
                frameData[current_frame]
                    .gRenderFinished
                    ->get_raw_object(),  // wait for gPass: not needed? maybe
                                         // implicit synchro is enough.
            },
            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            {frameData[current_frame]
                 .postRenderFinished->get_raw_object()},  // postPass is done
            frameData[current_frame]
                .renderingFence->get_raw_object()  // frame fence
            ));

    VkSemaphore wait_semaphores[] = {
        frameData[current_frame].postRenderFinished->get_raw_object()};
    VkSwapchainKHR swapchains[] = {app.swapchain().get_raw_object()};

    VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                  nullptr,
                                  1,
                                  wait_semaphores,
                                  1,
                                  swapchains,
                                  &image_index,
                                  nullptr};

    app.present_queue()->vkQueuePresentKHR(app.present_queue(), &present_info);

    current_frame = next_frame;
    next_frame = (next_frame + 1) % app.swapchain_images().size();
  }

  app.device()->vkDeviceWaitIdle(app.device());
  data->logger()->LogInfo("Application Shutdown");

  return 0;
}
