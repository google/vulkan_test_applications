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

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;
using Vector3 = mathfu::Vector<float, 3>;
namespace torus_model {
#include "torus_knot.obj.h"
}
const auto& torus_data = torus_model::model;

uint32_t torus_vertex_shader[] =
#include "write_timestamp.vert.spv"
    ;

uint32_t torus_fragment_shader[] =
#include "write_timestamp.frag.spv"
    ;

struct WriteTimestampFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> torus_descriptor_set_;
  containers::unique_ptr<vulkan::VkBufferView> timestamp_buf_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class WriteTimestampSample
    : public sample_application::Sample<WriteTimestampFrameData> {
 public:
  WriteTimestampSample(const entry::EntryData* data,
                       const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<WriteTimestampFrameData>(data->allocator(), data, 1, 512, 1, 1,
                                        sample_application::SampleOptions()
                                            .EnableDepthBuffer()
                                            .EnableMultisampling(),
                                        requested_features),
        torus_(data->allocator(), data->logger(), torus_data),
        grey_scale_(0u),
        num_frames_(0u) {
    // Check the timestamp valid bits for the render queue
    uint32_t queue_family_index = app()->render_queue().index();
    auto queue_family_properties =
        vulkan::GetQueueFamilyProperties(data->allocator(), app()->instance(),
                                         app()->device().physical_device());
    timestamp_valid_bits_ =
        queue_family_properties[queue_family_index].timestampValidBits;
  }

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    num_frames_ = static_cast<uint32_t>(num_swapchain_images);

    // Create a occlusion query pool that contains a query for each frame.
    query_pool_ = containers::make_unique<vulkan::VkQueryPool>(
        data_->allocator(),
        vulkan::CreateQueryPool(
            &app()->device(),
            {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0,
             VK_QUERY_TYPE_TIMESTAMP, uint32_t(num_swapchain_images), 0}));

    // Create a buffer to store the query results for each frame, and to be
    // used in the fragment shader.
    torus_.InitializeData(app(), initialization_buffer);

    torus_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    torus_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    torus_descriptor_set_layouts_[2] = {
        2,                                        // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
        1,                                        // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,             // stageFlags
        nullptr                                   // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(), app()->CreatePipelineLayout({{
                                torus_descriptor_set_layouts_[0],
                                torus_descriptor_set_layouts_[1],
                                torus_descriptor_set_layouts_[2],
                            }}));

    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {{
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
             },
             {
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
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    torus_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    torus_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               torus_vertex_shader);
    torus_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               torus_fragment_shader);
    torus_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
    torus_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    torus_pipeline_->SetRasterizationFill(VK_POLYGON_MODE_LINE);
    torus_pipeline_->SetCullMode(VK_CULL_MODE_NONE);
    torus_pipeline_->SetInputStreams(&torus_);
    torus_pipeline_->SetViewport(viewport());
    torus_pipeline_->SetScissor(scissor());
    torus_pipeline_->SetSamples(num_samples());
    torus_pipeline_->AddAttachment();
    torus_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(Vector3{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(Vector3{0.0, 0.0, -3.0}) *
        Mat44::FromScaleVector(Vector3{0.5, 0.5, 0.5});

    timestamp_data_ =
        containers::make_unique<vulkan::BufferFrameData<TimestampData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    timestamp_data_->data().value = 0;
  }

  virtual void InitializeFrameData(
      WriteTimestampFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Create the buffer view for the query pool result buffer so the buffer
    // can be used in the fragment shader
    frame_data->timestamp_buf_view_ = app()->CreateBufferView(
        timestamp_data_->get_buffer(), VK_FORMAT_R32_UINT,
        timestamp_data_->get_offset_for_frame(frame_index),
        timestamp_data_->aligned_data_size());

    // Buffer memory barriers for the query/descriptor buffer.
    VkBufferMemoryBarrier to_use_query_results{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // srcAccessMask
        VK_ACCESS_SHADER_READ_BIT,                // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        timestamp_data_->get_buffer(),            // buffer
        timestamp_data_->get_offset_for_frame(frame_index),  // offset
        timestamp_data_->aligned_data_size(),                // size
    };

    // Populate the command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->torus_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet({
                                    torus_descriptor_set_layouts_[0],
                                    torus_descriptor_set_layouts_[1],
                                    torus_descriptor_set_layouts_[2],
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

    VkWriteDescriptorSet write[2] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->torus_descriptor_set_,      // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            2,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos,                            // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
            nullptr,                                  // pNext
            *frame_data->torus_descriptor_set_,       // dstSet
            2,                                        // dstbinding
            0,                                        // dstArrayElement
            1,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
            nullptr,                                  // pImageInfo
            nullptr,                                  // pBufferInfo
            &frame_data->timestamp_buf_view_
                 ->get_raw_object()  // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 2, write, 0,
                                            nullptr);

    ::VkImageView raw_views[2] = {depth_view(frame_data),
                                  color_view(frame_data)};

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

    VkClearValue clears[2];
    vulkan::MemoryClear(&clears[0]);
    clears[0].depthStencil.depth = 1.0f;
    vulkan::MemoryClear(&clears[1]);

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

    // Reset the query for this frame in the query pool and then begin query,
    // and so the vkGetQueryPoolResults called before submitting rendering
    // commands won't hang.
    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                    nullptr, 1, &to_use_query_results, 0,
                                    nullptr);
    cmdBuffer->vkCmdResetQueryPool(cmdBuffer, *query_pool_,
                                   static_cast<uint32_t>(frame_index), 1);

    cmdBuffer->vkCmdWriteTimestamp(
        cmdBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, *query_pool_,
        static_cast<uint32_t>(frame_index));

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *torus_pipeline_);
    cmdBuffer->vkCmdSetLineWidth(cmdBuffer, 1.0f);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->torus_descriptor_set_->raw_set(), 0, nullptr);
    torus_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render * 0.1f) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.1f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      WriteTimestampFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);

    uint64_t time_stamp = 0;
    app()->device()->vkGetQueryPoolResults(
        app()->device(), *query_pool_, static_cast<uint32_t>(frame_index), 1u,
        sizeof(uint64_t), &time_stamp, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT);

    // Trim the time stamp value to an uint32.
    timestamp_data_->data().value = uint32_t(time_stamp);
    timestamp_data_->UpdateBuffer(queue, frame_index);

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

  // Return true if the sample for quering timestamp
  bool IsValidForTimestamp() { return timestamp_valid_bits_ != 0; }

 private:
  struct CameraData {
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  struct TimestampData {
    uint32_t value;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> torus_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  containers::unique_ptr<vulkan::VkQueryPool> query_pool_;
  VkDescriptorSetLayoutBinding torus_descriptor_set_layouts_[3];
  vulkan::VulkanModel torus_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
  containers::unique_ptr<vulkan::BufferFrameData<TimestampData>>
      timestamp_data_;

  uint32_t grey_scale_;
  uint32_t num_frames_;

  uint32_t timestamp_valid_bits_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  requested_features.fillModeNonSolid = VK_TRUE;
  WriteTimestampSample sample(data, requested_features);
  if (!sample.IsValidForTimestamp()) {
    data->logger()->LogInfo(
        "Disabled sample due to zero valid bits for timestamp in physical "
        "device queue family property");
  } else {
    sample.Initialize();

    while (!sample.should_exit() && !data->WindowClosing()) {
      sample.ProcessFrame();
    }
    sample.WaitIdle();
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
