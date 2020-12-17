// Copyright 2019 Google Inc.
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

namespace x_model {
const struct {
  size_t num_vertices;
  float positions[24];
  float uv[16];
  float normals[24];
  size_t num_indices;
  uint32_t indices[12];
} model = {8,
           /*positions*/
           {1.5f, 1.5f,  0.0f,  1.5f,  1.5f,  0.0f, -1.5f, -1.5f,
            0.0f, -1.5f, -1.5f, 0.0f,  -1.5f, 1.5f, 0.0f,  -1.5f,
            1.5f, 0.0f,  1.5f,  -1.5f, 0.0f,  1.5f, -1.5f, 0.0f},
           /*texture_coords*/
           {},
           /*normals*/
           {-1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,
            1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f,
            -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, -1.0f},
           12,
           /*indices*/
           {0, 2, 1, 1, 2, 3, 4, 5, 6, 6, 5, 7}};
}  // namespace x_model
const auto& x_cross_data = x_model::model;

uint32_t line_vertex_shader[] =
#include "line.vert.spv"
    ;

uint32_t line_fragment_shader[] =
#include "line.frag.spv"
    ;

struct StencilExportFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> line_descriptor_set_;
  vulkan::ImagePointer stencil_;
  containers::unique_ptr<vulkan::VkImageView> stencil_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class StencilExportSample
    : public sample_application::Sample<StencilExportFrameData> {
 public:
  StencilExportSample(const entry::EntryData* data)
      : data_(data),
        Sample<StencilExportFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableMultisampling(), {}, {},
            {VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME}),
        x_cross_(data->allocator(), data->logger(), x_cross_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    x_cross_.InitializeData(app(), initialization_buffer);

    line_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    line_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{line_descriptor_set_layouts_[0],
                                      line_descriptor_set_layouts_[1]}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference stencil_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

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
             },
             {
                 0,                                 // flags
                 VK_FORMAT_D24_UNORM_S8_UINT,       // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // storeOp
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // stenilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                 VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
             }},  // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &stencil_attachment,              // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    VkStencilOpState stencilOpState{VK_STENCIL_OP_KEEP,
                                    VK_STENCIL_OP_REPLACE,
                                    VK_STENCIL_OP_KEEP,
                                    VK_COMPARE_OP_GREATER,
                                    UINT32_MAX,
                                    UINT32_MAX,
                                    0};

    line_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    line_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              line_vertex_shader);
    line_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              line_fragment_shader);
    line_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    line_pipeline_->SetInputStreams(&x_cross_);
    line_pipeline_->SetViewport(viewport());
    line_pipeline_->SetScissor(scissor());
    line_pipeline_->SetSamples(num_samples());
    line_pipeline_->AddAttachment();
    line_pipeline_->DepthStencilState().depthTestEnable = false;
    line_pipeline_->DepthStencilState().stencilTestEnable = true;
    line_pipeline_->DepthStencilState().front = stencilOpState;
    line_pipeline_->Commit();

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

    model_data_->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
  }

  virtual void InitializeFrameData(
      StencilExportFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->line_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({line_descriptor_set_layouts_[0],
                                          line_descriptor_set_layouts_[1]}));

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
        *frame_data->line_descriptor_set_,       // dstSet
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

    VkImageCreateInfo image_create_info{
        /* sType = */
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_D24_UNORM_S8_UINT,
        /* extent = */
        {
            /* width = */ app()->swapchain().width(),
            /* height = */ app()->swapchain().height(),
            /* depth = */ app()->swapchain().depth(),
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ num_samples(),
        /* tiling = */
        VK_IMAGE_TILING_OPTIMAL,
        /* usage = */
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        /* sharingMode = */
        VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImageViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        VK_NULL_HANDLE,                            // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        VK_FORMAT_D24_UNORM_S8_UINT,               // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_views[2] = {color_view(frame_data), VK_NULL_HANDLE};

    frame_data->stencil_ = app()->CreateAndBindImage(&image_create_info);
    view_create_info.image = *frame_data->stencil_;

    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_views[1]);
    frame_data->stencil_view_ = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_views[1], nullptr, &app()->device()));

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

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clear[2];
    vulkan::MemoryClear(&clear[0]);
    vulkan::MemoryClear(&clear[1]);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clear                             // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *line_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->line_descriptor_set_->raw_set(), 0, nullptr);
    x_cross_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform = model_data_->data().transform *
                                    Mat44::FromRotationMatrix(Mat44::RotationZ(
                                        3.14f * time_since_last_render));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      StencilExportFrameData* frame_data) override {
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
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> line_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding line_descriptor_set_layouts_[2];
  vulkan::VulkanModel x_cross_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  StencilExportSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
