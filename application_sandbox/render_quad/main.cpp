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

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include <chrono>
#include "mathfu/matrix.h"
#include "mathfu/vector.h"

uint32_t fragment_shader[] =
#include "render_quad.frag.spv"
    ;

uint32_t vertex_shader[] =
#include "render_quad.vert.spv"
    ;

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

namespace simple_img {
#include "star.png.h"
}
const auto& src_data = simple_img::texture;

namespace {
// Unpack the source data according to the given formats and copy to the given
// destination.
void populateData(logging::Logger* log, uint8_t* dst, size_t size,
                  VkFormat staging_format, VkFormat target_format) {
  size_t target_pixel_width = 0;
  size_t staging_pixel_width = 0;
  // TODO: Handle more format
  switch (staging_format) {
    case VK_FORMAT_R8G8B8A8_UINT:
      staging_pixel_width = sizeof(uint32_t);
      break;
    case VK_FORMAT_R32_UINT:
      staging_pixel_width = sizeof(uint32_t);
      break;
    default:
      break;
  }
  switch (target_format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
      target_pixel_width = sizeof(uint32_t);
      break;
    case VK_FORMAT_D16_UNORM:
      target_pixel_width = sizeof(uint8_t) * 2;
      break;
    default:
      break;
  }
  // staging image must have a wider format than the target image to avoid
  // precision lost.
  if (target_pixel_width == 0 ) {
      log->LogInfo("Target image format not supported:", target_format);
  }
  if (staging_pixel_width == 0) {
      log->LogInfo("Staging image format not supported:", staging_format);
  }
  LOG_ASSERT(!=, log, 0, target_pixel_width);
  LOG_ASSERT(!=, log, 0, staging_pixel_width);
  LOG_ASSERT(>=, log, staging_pixel_width, target_pixel_width);

  size_t count = 0;
  size_t i = 0;
  while (i < size) {
    memcpy(&dst[i], &src_data.data[count++], sizeof(uint32_t));
    i += staging_pixel_width;
  }
}
}  // anonymous namespace

struct RenderQuadFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> render_command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  vulkan::ImagePointer color_staging_img_;
  vulkan::ImagePointer depth_staging_img_;
  containers::unique_ptr<vulkan::VkImageView> color_input_view_;
  containers::unique_ptr<vulkan::VkImageView> depth_input_view_;
  containers::unique_ptr<vulkan::DescriptorSet> descriptor_set_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class RenderQuadSample
    : public sample_application::Sample<RenderQuadFrameData> {
 public:
  RenderQuadSample(const entry::EntryData* data,
                   const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<RenderQuadFrameData>(
            // Each copy of color unpacked source data is: 400*400*4 bytes.
            data->allocator(), data, 10, 512, 10, 1,
            sample_application::SampleOptions().EnableDepthBuffer(),
            requested_features),
        plane_(data->allocator(), data->logger(), plane_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    plane_.InitializeData(app(), initialization_buffer);

    descriptor_set_layout_binding_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // desriptorType
        2,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{descriptor_set_layout_binding_}}));

    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference input_attachments[2] = {
        {2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    };

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {
                {
                    0,                                 // flags
                    depth_format(),                    // format
                    num_samples(),                     // samples
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Depth Attachment
                {
                    0,                                        // flags
                    render_format(),                          // format
                    num_samples(),                            // samples
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stenilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Color Attachment
                {
                    0,                                         // flags
                    VK_FORMAT_R8G8B8A8_UINT,               // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL   // finalLayout
                },  // Color Input Attachment
                {
                    0,                                         // flags
                    VK_FORMAT_R32_UINT,                        // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL   // finalLayout
                },  // Depth Input Attachment
            },      // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                2,                                // inputAttachmentCount
                input_attachments,                // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                      render_pass_.get(), 0));
    pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", vertex_shader);
    pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragment_shader);
    pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_->SetInputStreams(&plane_);
    pipeline_->SetScissor(scissor());
    pipeline_->SetViewport(viewport());
    pipeline_->SetSamples(num_samples());
    pipeline_->DepthStencilState().depthCompareOp = VK_COMPARE_OP_ALWAYS;
    pipeline_->AddAttachment();
    pipeline_->Commit();

    // TODO: initialize depth and color init data.
    color_data_ = containers::make_unique<vulkan::BufferFrameData<Data>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    depth_data_ = containers::make_unique<vulkan::BufferFrameData<Data>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    populateData(data_->logger(), color_data_->data().data.data(),
                 color_data_->data().data.size(), VK_FORMAT_R8G8B8A8_UINT,
                 app()->swapchain().format());
    populateData(data_->logger(), depth_data_->data().data.data(),
                 depth_data_->data().data.size(), VK_FORMAT_R32_UINT,
                 VK_FORMAT_D16_UNORM);
  }

  virtual void InitializeFrameData(
      RenderQuadFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->render_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    // Create the color and depth staging images
    VkImageCreateInfo img_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UINT,          // format
        {
            app()->swapchain().width(),   // width
            app()->swapchain().height(),  // height
            app()->swapchain().depth()    // depth
        },
        1,                        // mipLevels
        1,                        // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,    // samples
        VK_IMAGE_TILING_OPTIMAL,  // tiling
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };
    frame_data->color_staging_img_ = app()->CreateAndBindImage(&img_info);
    img_info.format = VK_FORMAT_R32_UINT;
    frame_data->depth_staging_img_ = app()->CreateAndBindImage(&img_info);

    // Create image views
    // color input view
    VkImageViewCreateInfo view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *frame_data->color_staging_img_,           // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        frame_data->color_staging_img_->format(),  // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    ::VkImageView raw_color_input_view;
    LOG_ASSERT(
        ==, data_->logger(), VK_SUCCESS,
        app()->device()->vkCreateImageView(app()->device(), &view_info, nullptr,
                                           &raw_color_input_view));
    frame_data->color_input_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data_->allocator(),
            vulkan::VkImageView(raw_color_input_view, nullptr,
                                &app()->device()));
    // depth input view
    view_info.image = *frame_data->depth_staging_img_;
    view_info.format = frame_data->depth_staging_img_->format();
    ::VkImageView raw_depth_input_view;
    LOG_ASSERT(
        ==, data_->logger(), VK_SUCCESS,
        app()->device()->vkCreateImageView(app()->device(), &view_info, nullptr,
                                           &raw_depth_input_view));
    frame_data->depth_input_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data_->allocator(),
            vulkan::VkImageView(raw_depth_input_view, nullptr,
                                &app()->device()));

    // Create a framebuffer for rendering
    VkImageView views[4] = {
        depth_view(frame_data),
        color_view(frame_data),
        *frame_data->color_input_view_,
        *frame_data->depth_input_view_,
    };
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        4,                                          // attachmentCount
        views,                                      // attachments
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

    // Update the descriptor set with input attachment info
    frame_data->descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({descriptor_set_layout_binding_}));
    VkDescriptorImageInfo input_attachment_infos[2] = {
        {
            VK_NULL_HANDLE,                            // sampler
            *frame_data->color_input_view_,            // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        },
        {
            VK_NULL_HANDLE,                            // sampler
            *frame_data->depth_input_view_,            // imageView
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
        },
    };
    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->descriptor_set_,            // dstSet
        0,                                       // dstBinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,     // descriptorType
        input_attachment_infos,                  // pImageInfo
        nullptr,                                 // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    // Recording commands: 1) copy data to staging image, 2) render staging
    // images to color and depth attachments
    (*frame_data->render_command_buffer_)
        ->vkBeginCommandBuffer(*frame_data->render_command_buffer_,
                               &sample_application::kBeginCommandBuffer);
    // Copy data from color/depth source buffer to the staging images.
    // Buffer barriers to src
    VkBufferMemoryBarrier color_data_to_src{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,         // sType
        nullptr,                                         // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,                    // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                     // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                         // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                         // dstQueueFamilyIndex
        color_data_->get_buffer(),                       // buffer
        color_data_->get_offset_for_frame(frame_index),  // offset
        color_data_->size(),                             // size
    };
    VkBufferMemoryBarrier depth_data_to_src{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,         // sType
        nullptr,                                         // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,                    // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                     // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                         // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                         // dstQueueFamilyIndex
        depth_data_->get_buffer(),                       // buffer
        depth_data_->get_offset_for_frame(frame_index),  // offset
        depth_data_->size(),                             // size
    };
    VkBufferMemoryBarrier bufs_to_src[2] = {color_data_to_src,
                                            depth_data_to_src};

    // Image barriers to dst
    VkImageMemoryBarrier color_input_undef_to_dst{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        VK_ACCESS_SHADER_READ_BIT,               // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        *frame_data->color_staging_img_,         // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkImageMemoryBarrier depth_input_undef_to_dst = color_input_undef_to_dst;
    depth_input_undef_to_dst.image = *frame_data->depth_staging_img_;
    VkImageMemoryBarrier imgs_to_dst[2] = {color_input_undef_to_dst,
                                           depth_input_undef_to_dst};
    (*frame_data->render_command_buffer_)
        ->vkCmdPipelineBarrier(
            *frame_data->render_command_buffer_,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_HOST_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_TRANSFER_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_TRANSFER_BIT,      // dstStageMask
            0,                                   // dependencyFlags
            0,                                   // memoryBarrierCount
            nullptr,                             // memoryBarrierCount
            2,                                   // bufferMemoryBarrierCount
            bufs_to_src,                         // pBufferMemoryBarriers
            2,                                   // imageMemoryBarrierCount
            imgs_to_dst                          // pImageMemoryBarriers
        );
    // copy from buf to img. The swapchain image must be larger in both
    // dimensions.
    LOG_ASSERT(>=, data_->logger(), app()->swapchain().width(), src_data.width);
    LOG_ASSERT(>=, data_->logger(), app()->swapchain().height(), src_data.height);
    uint32_t copy_width = src_data.width;
    uint32_t copy_height = src_data.height;
    VkBufferImageCopy copy_region{
        color_data_->get_offset_for_frame(frame_index),
        0,
        0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {copy_width, copy_height, 1},
    };
    (*frame_data->render_command_buffer_)
        ->vkCmdCopyBufferToImage(
            *frame_data->render_command_buffer_, color_data_->get_buffer(),
            *frame_data->color_staging_img_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    copy_region.bufferOffset = depth_data_->get_offset_for_frame(frame_index);
    (*frame_data->render_command_buffer_)
        ->vkCmdCopyBufferToImage(
            *frame_data->render_command_buffer_, depth_data_->get_buffer(),
            *frame_data->depth_staging_img_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    // staging image from dst to shader read
    VkImageMemoryBarrier color_input_dst_to_shader_read{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,              // srcAccessMask
        VK_ACCESS_SHADER_READ_BIT,                 // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,      // oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        *frame_data->color_staging_img_,           // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkImageMemoryBarrier depth_input_dst_to_shader_read =
        color_input_dst_to_shader_read;
    depth_input_dst_to_shader_read.image = *frame_data->depth_staging_img_;
    VkImageMemoryBarrier imgs_to_shader_read[2] = {
        color_input_dst_to_shader_read, depth_input_dst_to_shader_read};
    (*frame_data->render_command_buffer_)
        ->vkCmdPipelineBarrier(
            *frame_data->render_command_buffer_,
            VK_PIPELINE_STAGE_TRANSFER_BIT,         // srcStageMask
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  // dstStageMask
            0,                                      // dependencyFlags
            0,                                      // memoryBarrierCount
            nullptr,                                // memoryBarrierCount
            0,                                      // bufferMemoryBarrierCount
            nullptr,                                // pBufferMemoryBarriers
            2,                                      // imageMemoryBarrierCount
            imgs_to_shader_read                     // pImageMemoryBarriers
        );

    // Renders the content in the color/depth staging image to the color/depth
    // attachment images.
    VkRenderPassBeginInfo begin_first_render_pass = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        0,                                // clearValueCount
        nullptr                           // clears
    };

    (*frame_data->render_command_buffer_)
        ->vkCmdBeginRenderPass(*frame_data->render_command_buffer_,
                               &begin_first_render_pass,
                               VK_SUBPASS_CONTENTS_INLINE);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindPipeline(*frame_data->render_command_buffer_,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindDescriptorSets(*frame_data->render_command_buffer_,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  ::VkPipelineLayout(*pipeline_layout_), 0, 1,
                                  &frame_data->descriptor_set_->raw_set(), 0,
                                  nullptr);
    plane_.Draw(frame_data->render_command_buffer_.get());
    (*frame_data->render_command_buffer_)
        ->vkCmdEndRenderPass(*frame_data->render_command_buffer_);

    (*frame_data->render_command_buffer_)
        ->vkEndCommandBuffer(*frame_data->render_command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    // Do not update any data in this sample.
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      RenderQuadFrameData* frame_data) override {
    color_data_->UpdateBuffer(queue, frame_index);
    depth_data_->UpdateBuffer(queue, frame_index);
    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &frame_data->render_command_buffer_->get_command_buffer(),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  struct Data {
    std::array<uint8_t, sizeof(src_data.data)> data;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding_;

  containers::unique_ptr<vulkan::BufferFrameData<Data>> color_data_;
  containers::unique_ptr<vulkan::BufferFrameData<Data>> depth_data_;

  vulkan::VulkanModel plane_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  RenderQuadSample sample(data, requested_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->ShouldExit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
