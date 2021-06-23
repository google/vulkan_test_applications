// Copyright 2017 Google Inc.
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

#include "cube_render.h"
#include "render_quad.h"
#include "pre_quad.h"

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"

struct CubeSampleData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  vulkan::ImagePointer cubeRenderTargetImg;
  containers::unique_ptr<vulkan::VkImageView> cubeRenderTargetImgView;
  vulkan::ImagePointer starRenderTargetImg;
  containers::unique_ptr<vulkan::VkImageView> starRenderTargetImgView;
  CubeRenderData cubeRenderData;
  RenderQuadData renderQuadData;
  PreQuadData preQuadData;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class CubeSample : public sample_application::Sample<CubeSampleData> {
 public:
  CubeSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeSampleData>(
            data->allocator(), data, 10, 1024, 10, 1,
            sample_application::SampleOptions().EnableMultisampling().EnableDepthBuffer()),
            cube(data),
            quad(data),
            preQuad(data) {}


  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    PreQuadVulkanInfo preQuadVulkanInfo{};
    preQuadVulkanInfo.num_samples = num_samples();
    preQuadVulkanInfo.colorFormat = render_format();
    preQuadVulkanInfo.scissor = scissor();
    preQuadVulkanInfo.viewport = viewport();
    preQuad.InitializeQuadData(app(), data_->allocator(), preQuadVulkanInfo, initialization_buffer, num_swapchain_images);

    CubeVulkanInfo cubeVulkanInfo{};
    cubeVulkanInfo.num_samples = num_samples();
    cubeVulkanInfo.colorFormat = render_format();
    cubeVulkanInfo.depthFormat = depth_format();
    cubeVulkanInfo.scissor = scissor();
    cubeVulkanInfo.viewport = viewport();
    cube.InitializeCubeData(app(), data_->allocator(), cubeVulkanInfo, initialization_buffer, num_swapchain_images);

    QuadVulkanInfo quadVulkanInfo{};
    quadVulkanInfo.num_samples = num_samples();
    quadVulkanInfo.colorFormat = render_format();
    quadVulkanInfo.scissor = scissor();
    quadVulkanInfo.viewport = viewport();
    quad.InitializeQuadData(app(), data_->allocator(), quadVulkanInfo, initialization_buffer, num_swapchain_images);
  }

  void CreateIntermediateRenderTargets(CubeSampleData* frame_data) {
    // Create the staging image
    VkImageCreateInfo img_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UINT,              // format
        {
            app()->swapchain().width(),   // width
            app()->swapchain().height(),  // height
            app()->swapchain().depth()    // depth,
        },
        1,                        // mipLevels
        1,                        // arrayLayers
        num_samples(),            // samples
        VK_IMAGE_TILING_OPTIMAL,  // tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };

    frame_data->cubeRenderTargetImg = app()->CreateAndBindImage(&img_info);
    frame_data->starRenderTargetImg = app()->CreateAndBindImage(&img_info);

    // Create image views
    // color input view
    VkImageViewCreateInfo cube_rt_image_view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *frame_data->cubeRenderTargetImg,              // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        frame_data->cubeRenderTargetImg->format(),     // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_cube_input_view;
    LOG_ASSERT(
        ==, data_->logger(), VK_SUCCESS,
        app()->device()->vkCreateImageView(app()->device(), &cube_rt_image_view_info, nullptr,
                                           &raw_cube_input_view));
    frame_data->cubeRenderTargetImgView =
        containers::make_unique<vulkan::VkImageView>(data_->allocator(),
            vulkan::VkImageView(raw_cube_input_view, nullptr, &app()->device()));


    VkImageViewCreateInfo star_rt_image_view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *frame_data->starRenderTargetImg,              // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        frame_data->starRenderTargetImg->format(),     // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_star_input_view;
    LOG_ASSERT(
        ==, data_->logger(), VK_SUCCESS,
        app()->device()->vkCreateImageView(app()->device(), &star_rt_image_view_info, nullptr,
                                           &raw_star_input_view));
    frame_data->starRenderTargetImgView =
        containers::make_unique<vulkan::VkImageView>(data_->allocator(),
            vulkan::VkImageView(raw_star_input_view, nullptr, &app()->device()));
  }

  virtual void InitializeFrameData(CubeSampleData* frame_data, 
    vulkan::VkCommandBuffer* initialization_buffer,
    size_t frame_index) override {

    frame_data->command_buffer_ = 
      containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    CreateIntermediateRenderTargets(frame_data);

    preQuad.InitializeFrameData(
        app(),
        &frame_data->preQuadData,
        data_->allocator(),
        *frame_data->starRenderTargetImgView,
        frame_index);

    cube.InitializeFrameData(
        app(),
        &frame_data->cubeRenderData,
        data_->allocator(),
        *frame_data->starRenderTargetImgView,
        *frame_data->cubeRenderTargetImgView,
        depth_view(frame_data),
        frame_index);

    quad.InitializeFrameData(
        app(), 
        &frame_data->renderQuadData,
        data_->allocator(), 
        *frame_data->cubeRenderTargetImgView,
        color_view(frame_data),
        frame_index);

    cmdBuffer->vkBeginCommandBuffer(cmdBuffer, &sample_application::kBeginCommandBuffer);
    preQuad.RecordRenderCmds(app(), &frame_data->preQuadData, cmdBuffer, frame_index);
    cube.RecordRenderCmds(app(), &frame_data->cubeRenderData, cmdBuffer);
    quad.RecordRenderCmds(app(), &frame_data->renderQuadData, cmdBuffer, frame_index);
    cmdBuffer->vkEndCommandBuffer(cmdBuffer);
  }

  virtual void Update(float time_since_last_render) override {
    preQuad.Update(time_since_last_render);
    cube.Update(time_since_last_render);
    quad.Update(time_since_last_render);
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeSampleData* frame_data) override {
    preQuad.UpdateRenderData(queue, frame_index);
    cube.UpdateRenderData(queue, frame_index);
    quad.UpdateRenderData(queue, frame_index);
    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(frame_data->command_buffer_->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
    const entry::EntryData* data_;
    CubeRender cube;
    RenderQuad quad;
    PreQuad preQuad;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  CubeSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
