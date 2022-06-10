// Copyright 2022 Google Inc.
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
#include <cstddef>
#include <cstdint>
#include <fstream>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
#include "vulkan_core.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"
#include "vulkan_helpers/vulkan_texture.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

namespace simple_texture {
#include "star.png.h"
}

const auto& texture_data = simple_texture::texture;

struct CubeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
};

VkPhysicalDeviceHlslBindingSemanticsFeaturesEXT kFeat{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HLSL_BINDING_SEMANTICS_FEATURES_EXT,
    nullptr,
    VK_TRUE,
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class CubeSample : public sample_application::Sample<CubeFrameData> {
 public:
  CubeSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions()
                .EnableMultisampling()
                .SetVulkanApiVersion(VK_API_VERSION_1_1)
                .AddDeviceExtensionStructure(&kFeat),
            {}, {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_EXT_HLSL_BINDING_SEMANTICS_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        texture_(data->allocator(), data->logger(), texture_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    texture_.InitializeData(app(), initialization_buffer);

    // Query device features for inline_uniform_block.
    VkPhysicalDeviceHlslBindingSemanticsFeaturesEXT ext_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HLSL_BINDING_SEMANTICS_FEATURES_EXT,
        nullptr,
        VK_FALSE,
    };
    VkPhysicalDeviceFeatures2 features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &ext_features};

    app()->instance()->vkGetPhysicalDeviceFeatures2KHR(
        app()->device().physical_device(), &features);

    if (ext_features.hlslBindingSemantics == VK_FALSE) {
      data_->logger()->LogError(
          "hlslBindingSemantics not supported on this device.");
      exit(1);
    }

    VkPhysicalDeviceHlslBindingSemanticsPropertiesEXT dev_properties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HLSL_BINDING_SEMANTICS_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2 properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &dev_properties};

    app()->instance()->vkGetPhysicalDeviceProperties2KHR(
        app()->device().physical_device(), &properties);

    app()->instance()->GetLogger()->LogInfo(
        "VkPhysicalDeviceHlslBindingSemanticsPropertiesEXT properties:");
    app()->instance()->GetLogger()->LogInfo(
        "maxCombinedPipelineLayoutEntries: ",
        dev_properties.maxCombinedPipelineLayoutEntries);

    VkDescriptorHlslBindingInfoEXT pushConstantBindingInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_HLSL_BINDING_INFO_EXT,
        .pNext = nullptr,
        .registerValue = 0,
        .space = 0,
        .resourceType = VK_HLSL_RESOURCE_TYPE_CONSTANT_BUFFER_VIEW_EXT,

    };

    // VkDescriptorHlslBindingInfoEXT pushAddressBindingInfo = {
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_HLSL_BINDING_INFO_EXT,
    //     .pNext = nullptr,
    //     .registerValue = 3,
    //     .space = 2,
    //     .resourceType = VK_HLSL_RESOURCE_TYPE_SHADER_RESOURCE_VIEW_EXT,
    // };

    VkPipelineLayoutPushBindingCreateInfoEXT pipelineLayoutBindingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_PUSH_BINDING_CREATE_INFO_EXT,
        .pNext = nullptr,
        .pushConstantBindingInfoCount = 1,
        .pPushConstantBindingInfos = &pushConstantBindingInfo,
        .pushAddressBindingInfoCount = 0,
        .pPushAddressBindingInfos = nullptr,
        // .pushAddressBindingInfoCount = 1,
        // .pPushAddressBindingInfos = &pushAddressBindingInfo,
    };

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(MVPData)),
    };

    VkDescriptorSetLayoutHlslBindingCreateInfoEXT DescriptorSetlayoutCreateInfo = {
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_HLSL_BINDING_CREATE_INFO_EXT,
        .pNext = nullptr,
        .bindingCount = 1,
        .pHlslBindingInfos = &pushConstantBindingInfo,
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &DescriptorSetlayoutCreateInfo,
        .flags = VkDescriptorSetLayoutCreateFlags(
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_HLSL_BINDINGS_BIT_EXT),
        .bindingCount = 0,
        .pBindings = nullptr};

    ::VkDescriptorSetLayout ds_layout;
    LOG_ASSERT(==, app()->device()->GetLogger(), VK_SUCCESS,
               app()->device()->vkCreateDescriptorSetLayout(
                   app()->device(), &descriptor_set_layout_create_info, nullptr,
                   &ds_layout));

    descriptor_layout_ = containers::make_unique<vulkan::VkDescriptorSetLayout>(
        data_->allocator(),
        vulkan::VkDescriptorSetLayout(ds_layout, nullptr, &app()->device()));

    VkPipelineLayoutCreateInfo pineline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // sType
        &pipelineLayoutBindingCreateInfo,               // pNext
        VkPipelineLayoutCreateFlags(
            VK_PIPELINE_LAYOUT_CREATE_HLSL_BINDINGS_BIT_EXT),  // flags
        1,                                                     // setLayoutCount
        &ds_layout,                                            // pSetLayouts
        1,                   // pushConstantRangeCount
        &pushConstantRange,  // pPushConstantRanges
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(pineline_layout_create_info));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {{
                0,                                         // flags
                render_format(),                           // format
                num_samples(),                             // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stencilLoadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stencilStoreOp
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
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    sampler_ = containers::make_unique<vulkan::VkSampler>(
        data_->allocator(),
        vulkan::CreateSampler(&app()->device(), VK_FILTER_LINEAR,
                              VK_FILTER_LINEAR));

    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));

    auto cube_vertex_shader = LoadShaderFile("cube_vs.spv");
    auto cube_fragment_shader = LoadShaderFile("cube_ps.spv");
    cube_pipeline_->AddShader(
        VK_SHADER_STAGE_VERTEX_BIT, "VSMain",
        reinterpret_cast<uint32_t*>(cube_vertex_shader.data()),
        cube_vertex_shader.size() / 4);
    cube_pipeline_->AddShader(
        VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain",
        reinterpret_cast<uint32_t*>(cube_fragment_shader.data()),
        cube_fragment_shader.size() / 4);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(viewport());
    cube_pipeline_->SetScissor(scissor());
    cube_pipeline_->SetSamples(num_samples());
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    Mat44 projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    Mat44 transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
    mvp_matrices.projection = projection_matrix;
    mvp_matrices.transform = transform;
  }

  virtual void InitializationComplete() override {
    texture_.InitializationComplete();
  }

  virtual void InitializeFrameData(
      CubeFrameData* frame_data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    ::VkImageView raw_view = color_view(frame_data);

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        1,                                          // attachmentCount
        &raw_view,                                  // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data_->allocator(),
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));
  }

  std::vector<uint8_t> LoadShaderFile(std::string fileName) {
    std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);
    std::vector<uint8_t> shader_code;
    if (is.is_open()) {
      auto size = is.tellg();
      is.seekg(0, std::ios::beg);
      shader_code.resize(size);
      is.read(reinterpret_cast<char*>(shader_code.data()), size);
      is.close();
    } else {
      data_->logger()->LogError("Error: Could not open shader file \"",
                                fileName);
    }
    return shader_code;
  }

  virtual void Update(float time_since_last_render) override {
    mvp_matrices.transform =
        mvp_matrices.transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
  }

  void PrepareCommandBuffer(size_t frame_index, CubeFrameData* frame_data) {
    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdPushConstantsIndexedEXT(cmdBuffer, *pipeline_layout_.get(),
                                            VK_SHADER_STAGE_ALL_GRAPHICS, 0, 0,
                                            sizeof(MVPData), &mvp_matrices);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);

    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeFrameData* frame_data) override {
    PrepareCommandBuffer(frame_index, frame_data);

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
  struct MVPData {
    Mat44 projection;
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VkDescriptorSetLayout> descriptor_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  vulkan::VulkanModel cube_;
  vulkan::VulkanTexture texture_;
  containers::unique_ptr<vulkan::VkSampler> sampler_;
  int frame_count_ = 0;

  MVPData mvp_matrices;
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
