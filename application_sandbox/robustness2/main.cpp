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

uint32_t robustness2_vertex_shader[] =
#include "robustness2.vert.spv"
    ;

uint32_t robustness2_fragment_shader[] =
#include "robustness2.frag.spv"
    ;

struct Robustness2FrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> robustness2_descriptor_set_;
};

VkPhysicalDeviceRobustness2FeaturesEXT kRobustness2Features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT, nullptr,
    true /* robustBufferAccess2 */, true /* robustImageAccess2 */,
    true /* nullDescriptor */
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class Robustness2Sample
    : public sample_application::Sample<Robustness2FrameData> {
 public:
  Robustness2Sample(const entry::EntryData* data,
                    const VkPhysicalDeviceFeatures& request_features)
      : data_(data),
        Sample<Robustness2FrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().AddDeviceExtensionStructure(
                &kRobustness2Features),
            request_features,
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_EXT_ROBUSTNESS_2_EXTENSION_NAME}),
        robustness2_(data->allocator(), data->logger(), cube_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    robustness2_.InitializeData(app(), initialization_buffer);

    VkImageCreateInfo img_create_info{
        /* sType = */
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 128,
            /* height = */ 128,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */
        VK_IMAGE_TILING_OPTIMAL,
        /* usage = */
        VK_IMAGE_USAGE_STORAGE_BIT,
        /* sharingMode = */
        VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */
        VK_IMAGE_LAYOUT_UNDEFINED,
    };
    image_ = app()->CreateAndBindImage(&img_create_info);

    VkImageViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *image_,                                   // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        VK_FORMAT_R8G8B8A8_UNORM,                  // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_view;
    app()->device()->vkCreateImageView(app()->device(), &view_create_info,
                                       nullptr, &raw_view);
    image_view_ = containers::make_unique<vulkan::VkImageView>(
        data_->allocator(),
        vulkan::VkImageView(raw_view, nullptr, &app()->device()));

    robustness2_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    robustness2_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    robustness2_descriptor_set_layouts_[2] = {
        2,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,       // stageFlags
        nullptr                             // pImmutableSamplers
    };
    robustness2_descriptor_set_layouts_[3] = {
        3,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,       // stageFlags
        nullptr                             // pImmutableSamplers
    };
    robustness2_descriptor_set_layouts_[4] = {
        4,                                 // binding
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr                            // pImmutableSamplers
    };
    robustness2_descriptor_set_layouts_[5] = {
        5,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,       // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(), app()->CreatePipelineLayout(
                                {{robustness2_descriptor_set_layouts_[0],
                                  robustness2_descriptor_set_layouts_[1],
                                  robustness2_descriptor_set_layouts_[2],
                                  robustness2_descriptor_set_layouts_[3],
                                  robustness2_descriptor_set_layouts_[4],
                                  robustness2_descriptor_set_layouts_[5]}}));

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

    robustness2_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    robustness2_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                     robustness2_vertex_shader);
    robustness2_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                     robustness2_fragment_shader);
    robustness2_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    robustness2_pipeline_->SetInputStreams(&robustness2_);
    robustness2_pipeline_->SetViewport(viewport());
    robustness2_pipeline_->SetScissor(scissor());
    robustness2_pipeline_->SetSamples(num_samples());
    robustness2_pipeline_->AddAttachment();
    robustness2_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    robust_uniform_data_ =
        containers::make_unique<vulkan::BufferFrameData<RobustBufferData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    robust_storage_data_ =
        containers::make_unique<vulkan::BufferFrameData<RobustBufferData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});

    robust_uniform_data_->data().data[512] = 456;
    memset(robust_storage_data_->data().data, 0, 1024 * 4);

    VkPhysicalDeviceRobustness2PropertiesEXT robustness2_properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT, nullptr,
        0, 0};
    VkPhysicalDeviceProperties2 physical_device_properties2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        &robustness2_properties,  // pNext;
        {},                       // properties
    };
    app()->instance()->vkGetPhysicalDeviceProperties2KHR(
        app()->device().physical_device(), &physical_device_properties2);

    app()->instance()->GetLogger()->LogInfo(
        "Robust storage buffer access size alignment ",
        robustness2_properties.robustStorageBufferAccessSizeAlignment);
    app()->instance()->GetLogger()->LogInfo(
        "Robust uniform buffer access size alignment ",
        robustness2_properties.robustUniformBufferAccessSizeAlignment);
  }

  virtual void InitializeFrameData(
      Robustness2FrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->robustness2_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {robustness2_descriptor_set_layouts_[0],
                                     robustness2_descriptor_set_layouts_[1],
                                     robustness2_descriptor_set_layouts_[2],
                                     robustness2_descriptor_set_layouts_[3],
                                     robustness2_descriptor_set_layouts_[4],
                                     robustness2_descriptor_set_layouts_[5]}));

    VkDescriptorBufferInfo buffer_infos[3] = {
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

    VkDescriptorBufferInfo uniform_buffer = {
        robust_uniform_data_->get_buffer(),  // buffer
        0,                                   // offset
        256 * 4,                             // range
    };

    VkDescriptorBufferInfo storage_buffer = {
        robust_storage_data_->get_buffer(),  // buffer
        0,                                   // offset
        256 * 4,                             // range
    };

    VkDescriptorImageInfo image_info = {VK_NULL_HANDLE, *image_view_,
                                        VK_IMAGE_LAYOUT_GENERAL};

    VkDescriptorBufferInfo null_buffer = {
        VK_NULL_HANDLE,
        0,
        VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet writes[5] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
            nullptr,                                   // pNext
            *frame_data->robustness2_descriptor_set_,  // dstSet
            0,                                         // dstbinding
            0,                                         // dstArrayElement
            2,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // descriptorType
            nullptr,                                   // pImageInfo
            buffer_infos,                              // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
            nullptr,                                   // pNext
            *frame_data->robustness2_descriptor_set_,  // dstSet
            2,                                         // dstbinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // descriptorType
            nullptr,                                   // pImageInfo
            &uniform_buffer,                           // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
            nullptr,                                   // pNext
            *frame_data->robustness2_descriptor_set_,  // dstSet
            3,                                         // dstbinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         // descriptorType
            nullptr,                                   // pImageInfo
            &storage_buffer,                           // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
            nullptr,                                   // pNext
            *frame_data->robustness2_descriptor_set_,  // dstSet
            4,                                         // dstbinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          // descriptorType
            &image_info,                               // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
            nullptr,                                   // pNext
            *frame_data->robustness2_descriptor_set_,  // dstSet
            5,                                         // dstbinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         // descriptorType
            nullptr,                                   // pImageInfo
            &null_buffer,                              // pBufferInfo
            nullptr,                                   // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 5, writes, 0,
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

    VkImageMemoryBarrier undef_to_general{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_SHADER_WRITE_BIT,  // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,       // oldLayout
        VK_IMAGE_LAYOUT_GENERAL,         // newLayout
        VK_QUEUE_FAMILY_IGNORED,         // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,         // dstQueueFamilyIndex
        *image_,                         // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,      // srcStageMask
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  // dstStageMask
        0,                                      // dependencyFlags
        0, nullptr, 0, nullptr, 1, &undef_to_general);

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
                                 *robustness2_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->robustness2_descriptor_set_->raw_set(), 0, nullptr);
    robustness2_.Draw(&cmdBuffer);
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
                      Robustness2FrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);
    robust_uniform_data_->UpdateBuffer(queue, frame_index);

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

  struct RobustBufferData {
    uint32_t data[1024];
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> robustness2_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding robustness2_descriptor_set_layouts_[6];
  vulkan::VulkanModel robustness2_;
  vulkan::ImagePointer image_;
  containers::unique_ptr<vulkan::VkImageView> image_view_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
  containers::unique_ptr<vulkan::BufferFrameData<RobustBufferData>>
      robust_uniform_data_;
  containers::unique_ptr<vulkan::BufferFrameData<RobustBufferData>>
      robust_storage_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures request_features = {0};
  request_features.fragmentStoresAndAtomics = VK_TRUE;
  request_features.robustBufferAccess = VK_TRUE;

  Robustness2Sample sample(data, request_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
