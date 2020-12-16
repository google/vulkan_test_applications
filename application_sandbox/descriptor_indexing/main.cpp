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

#include <chrono>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
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

uint32_t descriptor_indexing_vertex_shader[] =
#include "descriptor_indexing.vert.spv"
    ;

uint32_t descriptor_indexing_fragment_shader[] =
#include "descriptor_indexing.frag.spv"
    ;

namespace simple_texture {
#include "star.png.h"
}

const auto& texture_data = simple_texture::texture;

struct TexturedCubeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class TexturedCubeSample
    : public sample_application::Sample<TexturedCubeFrameData> {
 public:
  TexturedCubeSample(const entry::EntryData* data,
                     const VkPhysicalDeviceFeatures& requested_features,
                     void* device_next)
      : data_(data),
        Sample<TexturedCubeFrameData>{
            data->allocator(),
            data,
            32,
            512,
            32,
            32,
            sample_application::SampleOptions().AddDeviceExtensionStructure(
                device_next),
            requested_features,
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_KHR_MAINTENANCE3_EXTENSION_NAME,
             VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME}},
        cube_(data->allocator(), data->logger(), cube_data),
        texture_{data->allocator(), data->logger(), texture_data} {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    texture_.InitializeData(app(), initialization_buffer);

    ::VkImageView raw_view;

    VkImageViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        texture_.image(),                          // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        texture_data.format,                       // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_view);
    image_views_[0] = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_view, nullptr, &app()->device()));
    view_create_info.components = {
        VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO,
        VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_A};
    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_view);
    image_views_[1] = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_view, nullptr, &app()->device()));
    view_create_info.components = {
        VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_A};
    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_view);
    image_views_[2] = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_view, nullptr, &app()->device()));
    view_create_info.components = {
        VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO,
        VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_view);
    image_views_[3] = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_view, nullptr, &app()->device()));

    cube_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_descriptor_set_layouts_[2] = {
        2,                             // binding
        VK_DESCRIPTOR_TYPE_SAMPLER,    // descriptorType
        1,                             // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,  // stageFlags
        nullptr                        // pImmutableSamplers
    };
    cube_descriptor_set_layouts_[3] = {
        3,                                 // binding
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  // descriptorType
        4,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr                            // pImmutableSamplers
    };

    sampler_ = containers::make_unique<vulkan::VkSampler>(
        data_->allocator(),
        vulkan::CreateSampler(&app()->device(), VK_FILTER_LINEAR,
                              VK_FILTER_LINEAR));

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{cube_descriptor_set_layouts_[0], cube_descriptor_set_layouts_[1],
              cube_descriptor_set_layouts_[2],
              cube_descriptor_set_layouts_[3]}}));

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
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              descriptor_indexing_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              descriptor_indexing_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(viewport());
    cube_pipeline_->SetScissor(scissor());
    cube_pipeline_->SetSamples(num_samples());
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix[0] =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{-0.5f, -0.5f, 0.0f}) *
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);
    camera_data_->data().projection_matrix[1] =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{-0.5f, 0.5f, 0.0f}) *
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);
    camera_data_->data().projection_matrix[2] =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.5f, -0.5f, 0.0f}) *
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);
    camera_data_->data().projection_matrix[3] =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.5f, 0.5f, 0.0f}) *
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
  }

  virtual void InitializationComplete() override {
    texture_.InitializationComplete();
  }

  virtual void InitializeFrameData(
      TexturedCubeFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet({
                                    cube_descriptor_set_layouts_[0],
                                    cube_descriptor_set_layouts_[1],
                                    cube_descriptor_set_layouts_[2],
                                    cube_descriptor_set_layouts_[3],
                                }));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data_->get_buffer(),                       // buffer
            camera_data_->get_offset_for_frame(frame_index),  // offset
            camera_data_->size(),                             // range
        },
        {
            model_data_->get_buffer(),                       // buffer
            model_data_->get_offset_for_frame(frame_index),  // offset
            model_data_->size(),                             // range
        }};

    VkDescriptorImageInfo sampler_info = {
        *sampler_,                 // sampler
        VK_NULL_HANDLE,            // imageView
        VK_IMAGE_LAYOUT_UNDEFINED  //  imageLayout
    };

    VkDescriptorImageInfo texture_info[4] = {
        {
            VK_NULL_HANDLE,                            // sampler
            *image_views_[0],                          // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        },
        {
            VK_NULL_HANDLE,                            // sampler
            *image_views_[1],                          // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        },
        {
            VK_NULL_HANDLE,                            // sampler
            *image_views_[2],                          // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        },
        {
            VK_NULL_HANDLE,                            // sampler
            *image_views_[3],                          // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        }};

    VkWriteDescriptorSet writes[5] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_descriptor_set_,       // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            &buffer_infos[0],                        // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_descriptor_set_,       // dstSet
            1,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            &buffer_infos[1],                        // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_descriptor_set_,       // dstSet
            2,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_SAMPLER,              // descriptorType
            &sampler_info,                           // pImageInfo
            nullptr,                                 // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_descriptor_set_,       // dstSet
            3,                                       // dstbinding
            0,                                       // dstArrayElement
            4,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        // descriptorType
            texture_info,                            // pImageInfo
            nullptr,                                 // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 4, writes, 0,
                                            nullptr);

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

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_descriptor_set_->raw_set(), 0, nullptr);
    cube_.DrawInstanced(&cmdBuffer, 4);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      TexturedCubeFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);

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
  struct CameraData {
    Mat44 projection_matrix[4];
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[4];
  vulkan::VulkanModel cube_;
  vulkan::VulkanTexture texture_;
  containers::unique_ptr<vulkan::VkImageView> image_views_[4];
  containers::unique_ptr<vulkan::VkSampler> sampler_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  requested_features.vertexPipelineStoresAndAtomics = true;
  VkPhysicalDeviceDescriptorIndexingFeaturesEXT desciptor_index_features{};
  desciptor_index_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
  desciptor_index_features.runtimeDescriptorArray = true;
  desciptor_index_features.shaderSampledImageArrayNonUniformIndexing = true;
  TexturedCubeSample sample(data, requested_features,
                            &desciptor_index_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
