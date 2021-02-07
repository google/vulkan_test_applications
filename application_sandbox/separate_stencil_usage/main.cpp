// Copyright 2021 Google Inc.
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

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

uint32_t cube_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "cube.frag.spv"
    ;

namespace floor_model {
#include "floor.obj.h"
}
const auto& floor_data = floor_model::model;

uint32_t floor_vertex_shader[] =
#include "floor.vert.spv"
    ;

uint32_t floor_fragment_shader[] =
#include "floor.frag.spv"
    ;

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

uint32_t plane_vertex_shader[] =
#include "plane.vert.spv"
    ;

uint32_t plane_fragment_shader[] =
#include "plane.frag.spv"
    ;

const VkFormat kDepthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

struct SeparateStencilUsageFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> read_stencil_descriptor_set_;

  // The default support for building a depth/stencil image is insufficient
  // for this sample, we declare another one here. We also declar two views
  // of the image, one that is both depth/stencil and then a stencil only.
  vulkan::ImagePointer depth_stencil_image_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_view_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_view_stencil_only_;
};

class SeparateStencilUsageSample
    : public sample_application::Sample<SeparateStencilUsageFrameData> {
 public:
  SeparateStencilUsageSample(const entry::EntryData* data)
      : data_(data),
        Sample<SeparateStencilUsageFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableVulkan11(), {0}, {},
            {VK_EXT_SEPARATE_STENCIL_USAGE_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        floor_(data->allocator(), data->logger(), floor_data),
        plane_(data->allocator(), data->logger(), plane_data) {}

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    floor_.InitializeData(app(), initialization_buffer);
    plane_.InitializeData(app(), initialization_buffer);

    descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{descriptor_set_layouts_[0], descriptor_set_layouts_[1]}}));

    read_stencil_pipeline_layout_bindings_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // descriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stage
        nullptr                               // pImmutableSamplers
    };

    read_stencil_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->allocator(),
            app()->CreatePipelineLayout(
                {{read_stencil_pipeline_layout_bindings_}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference read_stencil_attachment = {
        1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {
                {
                    0,                                         // flags
                    render_format(),                           // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
                },
                {
                    0,                             // flags
                    kDepthStencilFormat,           // format
                    num_samples(),                 // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,   // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,  // storeOp
                    VK_ATTACHMENT_LOAD_OP_CLEAR,   // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_STORE,  // stencilStoreOp
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout
                },
            },  // AttachmentDescriptions
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
            ));

    read_stencil_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {
                {
                    0,                                         // flags
                    render_format(),                           // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
                },
                {
                    0,                                         // flags
                    kDepthStencilFormat,                       // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // storeOp
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
                },
            },  // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                1,                                // inputAttachmentCount
                &read_stencil_attachment,         // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    // Initialize cube pipeline
    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              cube_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              cube_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(viewport());
    cube_pipeline_->SetScissor(scissor());
    cube_pipeline_->SetSamples(num_samples());
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    // Initialize floor pipeline
    floor_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    floor_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               floor_vertex_shader);
    floor_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               floor_fragment_shader);
    floor_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    floor_pipeline_->SetInputStreams(&floor_);
    floor_pipeline_->SetViewport(viewport());
    floor_pipeline_->SetScissor(scissor());
    floor_pipeline_->SetSamples(num_samples());
    floor_pipeline_->AddAttachment();
    floor_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    floor_pipeline_->DepthStencilState().front.compareOp = VK_COMPARE_OP_ALWAYS;
    floor_pipeline_->DepthStencilState().front.passOp = VK_STENCIL_OP_REPLACE;
    floor_pipeline_->DepthStencilState().front.reference = 0xFF;
    floor_pipeline_->DepthStencilState().front.writeMask = 0xFF;
    floor_pipeline_->Commit();

    read_stencil_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(read_stencil_pipeline_layout_.get(),
                                          read_stencil_render_pass_.get(), 0));
    read_stencil_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                      plane_vertex_shader);
    read_stencil_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                      plane_fragment_shader);
    read_stencil_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // The stencil readback is limited to the right hand side of the screen,
    // to show a sort of color/stencil side-by-side
    auto vp = viewport();
    vp.x = vp.width / 2;
    vp.width = vp.width / 2;
    read_stencil_pipeline_->SetViewport(vp);
    read_stencil_pipeline_->SetScissor(scissor());
    read_stencil_pipeline_->SetInputStreams(&plane_);
    read_stencil_pipeline_->SetSamples(num_samples());
    read_stencil_pipeline_->AddAttachment();
    read_stencil_pipeline_->Commit();

    // Transformation data for viewing and cube/floor rotation.
    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * 0.2f));
  }

  virtual void InitializeFrameData(
      SeparateStencilUsageFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Initalize the depth stencil image and the image view. This include
    // a pNext with the separate stencil usage. This is what differs
    // from the stock depth/stencil support. The depth aspect only
    // support VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT while the
    // stencil aspect has VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT and
    // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT.
    // VkImageStencilUsageCreateInfo stencil_usage_create_info = {
    //    VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO,  // sType;
    //    nullptr,                                            // pNext;
    //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
    //        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // stencilUsage;
    //};
    VkImageCreateInfo depth_stencil_image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,              //&stencil_usage_create_info,           // pNext
        0,                    // flags
        VK_IMAGE_TYPE_2D,     // imageType
        kDepthStencilFormat,  // format
        {
            app()->swapchain().width(),
            app()->swapchain().height(),
            app()->swapchain().depth(),
        },                        // extent
        1,                        // mipLevels
        1,                        // arrayLayers
        num_color_samples(),      // samples
        VK_IMAGE_TILING_OPTIMAL,  // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };
    frame_data->depth_stencil_image_ =
        app()->CreateAndBindImage(&depth_stencil_image_create_info);
    frame_data->depth_stencil_view_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1});
    frame_data->depth_stencil_view_stencil_only_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

    // Need to transition to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    // for the first renderpass
    VkImageMemoryBarrier depth_stencil_image_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,            // sType
        nullptr,                                           // pNext
        0,                                                 // srcAccessMask
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,      // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                         // oldLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,            // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,            // dstQueueFamilyIndex
        *frame_data->depth_stencil_image_,  // image
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}};

    (*initialization_buffer)
        ->vkCmdPipelineBarrier(
            (*initialization_buffer), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &depth_stencil_image_barrier);

    // Initialize the descriptor sets
    frame_data->descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {descriptor_set_layouts_[0], descriptor_set_layouts_[1]}));

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

    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->descriptor_set_,            // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    frame_data->read_stencil_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {read_stencil_pipeline_layout_bindings_}));

    VkDescriptorImageInfo image_info = {
        VK_NULL_HANDLE,                                 // sampler
        *frame_data->depth_stencil_view_stencil_only_,  // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL        // imageLayout
    };

    VkWriteDescriptorSet read_stencil_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,     // sType
        nullptr,                                    // pNext
        *frame_data->read_stencil_descriptor_set_,  // dstSet
        0,                                          // dstbinding
        0,                                          // dstArrayElement
        1,                                          // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,        // descriptorType
        &image_info,                                // pImageInfo
        nullptr,                                    // pBufferInfo
        nullptr,                                    // pTexelBufferView
    };
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1,
                                            &read_stencil_write, 0, nullptr);

    ::VkImageView raw_views[2] = {color_view(frame_data),
                                  *frame_data->depth_stencil_view_};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        2,                                          // attachmentCount
        raw_views,                                  // attachments
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

    // Populate the render command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);
    cmdBuffer->vkBeginCommandBuffer(cmdBuffer,
                                    &sample_application::kBeginCommandBuffer);

    VkClearValue clears[2];
    clears[0].color = {0.5f, 1.0f, 1.0f, 1.0f};
    clears[1].depthStencil.depth = 1.0f;
    clears[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *floor_pipeline_);
    floor_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    pass_begin.renderPass = *read_stencil_render_pass_;
    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*read_stencil_pipeline_layout_), 0, 1,
        &frame_data->read_stencil_descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *read_stencil_pipeline_);
    plane_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    cmdBuffer->vkEndCommandBuffer(cmdBuffer);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform = model_data_->data().transform *
                                    Mat44::FromRotationMatrix(Mat44::RotationY(
                                        3.14f * time_since_last_render * 0.5f));
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      SeparateStencilUsageFrameData* frame_data) override {
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
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> read_stencil_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> floor_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> read_stencil_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> read_stencil_render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layouts_[2];
  VkDescriptorSetLayoutBinding read_stencil_pipeline_layout_bindings_ = {};
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel floor_;
  vulkan::VulkanModel plane_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  SeparateStencilUsageSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
