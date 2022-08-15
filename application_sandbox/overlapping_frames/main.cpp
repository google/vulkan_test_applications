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
// Here, each frame is rendered with two render passes: first, the "gbuffer"
// render pass renders a triangle, then the "postprocessing" render pass inverts
// the framebuffer colors. At each main iteration loop, the gbuffer render pass
// of frame N+1 and the postprocessing render pass of frame N are run, such that
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
// thus implementing overlapping frame preparation. In practice, we need at
// least 2 swapchain images, and we use frame indexes within [0 .. number of
// swapchain images].
//
// Vulkan synchronization: for a given frame index, the synchronization overview
// is:
//
// 1. wait for rendering fence
// 2. submit gbuffer: signals gbuffer semaphore gbuffer
// 3. acquire swapchain image: signals swapchain image sempahores
// 4. submit postprocessing: wait for gbuffer and swapchain image semaphores,
//    signals postprocessing semaphore and rendering fence.
// 5. present: wait on postprocessing semaphore
//
// The semaphores make sure gbuffer, postprocessing and present are synchronized
// on the device-side. A fence is also needed when we start a new frame on the
// same frame index, to prevent host-side editing of the gbuffer rendering
// resources while it could still be running for the previous use of this frame
// index. On a simple rendering app, one tend to use the vkQueuePresent fence to
// make sure not to edit the rendering resources while they may still be in use.
// Here, gbuffer output is consumed only by postprocessing, hence as soon as
// postprocessing is terminated, we can edit and submit the gbuffer.

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
}  // namespace gBuffer

namespace postBuffer {
uint32_t vert[] =
#include "postprocessing.vert.spv"
    ;

uint32_t frag[] =
#include "postprocessing.frag.spv"
    ;
}  // namespace postBuffer

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
  VkAttachmentReference color_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  vulkan::VkRenderPass render_pass = app->CreateRenderPass(
      {{
          0,                                 // flags
          app->swapchain().format(),         // format
          VK_SAMPLE_COUNT_1_BIT,             // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
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

  for (size_t i = 0; i < app->swapchain_images().size(); i++) {
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

  for (size_t i = 0; i < app->swapchain_images().size(); i++) {
    VkImage& swapchain_image = app->swapchain_images()[i];

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

  for (size_t i = 0; i < images.size(); i++) {
    auto sampler_image = images[i].get();

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

  for (size_t i = 0; i < app->swapchain_images().size(); i++) {
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        render_pass,                                // renderPass
        1,                                          // attachmentCount
        &image_views[i].get_raw_object(),           // attachments
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
  data->logger()->LogInfo("Start app: overlapping_frames");

  const uint32_t min_swapchain_image_count = 2;

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data,
      vulkan::VulkanApplicationOptions().SetMinSwapchainImageCount(
          min_swapchain_image_count),
      {}, {});

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

  // gbuffer render pass
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

  // Post render pass
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

  // Frame data
  containers::vector<FrameData> frame_data(data->allocator());
  frame_data.resize(app.swapchain_images().size());

  for (size_t i = 0; i < app.swapchain_images().size(); i++) {
    frame_data[i].gCommandBuffer =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());
    frame_data[i].postCommandBuffer =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());
    frame_data[i].gRenderFinished =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frame_data[i].imageAcquired = containers::make_unique<vulkan::VkSemaphore>(
        data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frame_data[i].postRenderFinished =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&app.device()));
    frame_data[i].renderingFence = containers::make_unique<vulkan::VkFence>(
        data->allocator(), vulkan::CreateFence(&app.device(), 1));
    frame_data[i].descriptorSet =
        containers::make_unique<vulkan::DescriptorSet>(
            data->allocator(),
            buildDescriptorSet(&app, sampler, g_image_views[i]));
  }

  // Clear with bright red such that the post-processing render pass results
  // in a rather dark purple.
  VkClearValue clear_color = {1.0f, 0.8f, 0.8f, 1.0f};

  // In each iteration, we track the current and next frame. We also use time
  // to rotate the triangle.
  uint32_t current_frame = 0;
  uint32_t next_frame = 1;
  uint32_t image_index;
  std::chrono::steady_clock::time_point start_time_point =
      std::chrono::steady_clock::now();
  const float triangle_speed = 0.01f;
  GeometryPushConstantData g_push_constant_data{0.0f};

  // Run the gbuffer render pass of the very first frame before entering the
  // main loop, to initialize the interleaving work.
  app.device()->vkResetFences(
      app.device(), 1,
      &frame_data[current_frame].renderingFence->get_raw_object());

  VkRenderPassBeginInfo g_pass_begin{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      g_render_pass,
      g_framebuffers[current_frame].get_raw_object(),
      {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
      1,
      &clear_color};

  auto& cmd_buf = frame_data[current_frame].gCommandBuffer;
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
                 {// Synchro: signal once gbuffer is terminated
                  frame_data[current_frame].gRenderFinished->get_raw_object()},
                 VK_NULL_HANDLE));

  app.device()->vkWaitForFences(app.device(), 1, &init_fence.get_raw_object(),
                                VK_TRUE, UINT64_MAX);

  // main loop
  while (!data->WindowClosing()) {
    // # Step 1: Prepare and submit gbuffer render pass for next_frame

    // Synchro: wait on the rendering fence. This is necessary to make sure the
    // previous postprocessing render pass on this frame index has terminated,
    // since postprocessing consumes gbuffer results, and here we are about to
    // edit gbuffer rendering resources.
    app.device()->vkWaitForFences(
        app.device(), 1,
        &frame_data[next_frame].renderingFence->get_raw_object(), VK_TRUE,
        UINT64_MAX);
    app.device()->vkResetFences(
        app.device(), 1,
        &frame_data[next_frame].renderingFence->get_raw_object());

    // Update push constants
    std::chrono::steady_clock::time_point current_time_point =
        std::chrono::steady_clock::now();
    float time_lapse = std::chrono::duration_cast<std::chrono::milliseconds>(
                           current_time_point - start_time_point)
                           .count();
    g_push_constant_data.time = triangle_speed * time_lapse;

    // Write command buffer
    auto& geometry_buf = frame_data[next_frame].gCommandBuffer;
    auto& geo_ref = *geometry_buf;

    app.BeginCommandBuffer(geometry_buf.get());

    VkRenderPassBeginInfo g_pass_begin{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        g_render_pass,
        g_framebuffers[next_frame].get_raw_object(),
        {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
        1,
        &clear_color};

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
                   {// Synchro: signal gBuffer is done
                    frame_data[next_frame].gRenderFinished->get_raw_object()},
                   VK_NULL_HANDLE));

    // # Step 2: prepare and submit postprocessing render pass for
    // current_frame, and present this frame.

    // This render pass renders into the swapchain image.
    app.device()->vkAcquireNextImageKHR(
        app.device(), app.swapchain().get_raw_object(), UINT64_MAX,
        frame_data[current_frame].imageAcquired->get_raw_object(),
        static_cast<VkFence>(VK_NULL_HANDLE), &image_index);

    auto& post_cmd_buf = frame_data[current_frame].postCommandBuffer;
    vulkan::VkCommandBuffer& post_ref_cmd = *post_cmd_buf;

    app.BeginCommandBuffer(post_cmd_buf.get());
    vulkan::RecordImageLayoutTransition(
        app.swapchain_images()[image_index],
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, VK_IMAGE_LAYOUT_UNDEFINED, 0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, post_cmd_buf.get());

    VkRenderPassBeginInfo post_pass_begin{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        post_render_pass,
        post_framebuffers[current_frame].get_raw_object(),
        {{0, 0}, {app.swapchain().width(), app.swapchain().height()}},
        1,
        &clear_color};

    post_ref_cmd->vkCmdBeginRenderPass(post_ref_cmd, &post_pass_begin,
                                       VK_SUBPASS_CONTENTS_INLINE);
    post_ref_cmd->vkCmdBindDescriptorSets(
        post_ref_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post_pipeline_layout, 0,
        1, &frame_data[current_frame].descriptorSet->raw_set(), 0, nullptr);
    post_ref_cmd->vkCmdBindPipeline(
        post_ref_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post_pipeline);
    screen.Draw(&post_ref_cmd);
    post_ref_cmd->vkCmdEndRenderPass(post_ref_cmd);
    LOG_ASSERT(
        ==, data->logger(), VK_SUCCESS,
        app.EndAndSubmitCommandBuffer(
            &post_ref_cmd, &app.render_queue(),
            {
                // Synchro: wait for swapchain image
                frame_data[current_frame].imageAcquired->get_raw_object(),
                // Synchro: wait for the gbuffer render pass
                frame_data[current_frame].gRenderFinished->get_raw_object(),
            },
            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            {// Synchro: signal postprocessing is done
             frame_data[current_frame].postRenderFinished->get_raw_object()},
            // Synchro: signal rendering is done
            frame_data[current_frame].renderingFence->get_raw_object()));

    // Present current_frame
    VkSemaphore wait_semaphores[] = {
        // Synchro: wait on postprocessing to be finished
        frame_data[current_frame].postRenderFinished->get_raw_object()};
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

    // Update frame indexes
    current_frame = next_frame;
    next_frame = (next_frame + 1) % app.swapchain_images().size();
  }

  // Terminate
  app.device()->vkDeviceWaitIdle(app.device());
  data->logger()->LogInfo("Application Shutdown");

  return 0;
}
