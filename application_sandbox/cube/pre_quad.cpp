#include "pre_quad.h"

namespace quad_model {
#include "fullscreen_quad.obj.h"
}

namespace {
    uint32_t quad_vertex_shader[] =
    #include "quad.vert.spv"
        ;

    uint32_t quad_fragment_shader[] =
    #include "quad.frag.spv"
        ;
    const auto& quad_data = quad_model::model;

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
}

PreQuad::PreQuad(const entry::EntryData* data)
    : quad_(data->allocator(), data->logger(), quad_data) {}

void PreQuad::InitializeQuadData(vulkan::VulkanApplication* app,
    containers::Allocator* allocator,
    PreQuadVulkanInfo vulkanInfo,
    vulkan::VkCommandBuffer* initialization_buffer,
    size_t num_swapchain_images) {
    quad_.InitializeData(app, initialization_buffer);

    descriptor_set_layout_binding_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // desriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator, app->CreatePipelineLayout({{descriptor_set_layout_binding_}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference input_attachment = {
        1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        allocator,
        app->CreateRenderPass(
            {
                {
                    0,                                        // flags
                    vulkanInfo.colorFormat,                   // format
                    vulkanInfo.num_samples,                   // samples
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // initialLayout
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout
                },  // Color Attachment
                {
                    0,                                         // flags
                    VK_FORMAT_R8G8B8A8_UINT,                   // format
                    vulkanInfo.num_samples,                    // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stencilStoreOp
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL   // finalLayout
                },  // Input Attachment
            },      // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                1,                                // inputAttachmentCount
                &input_attachment,                // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        allocator,
        app->CreateGraphicsPipeline(pipeline_layout_.get(), render_pass_.get(), 0));
    pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",quad_vertex_shader);
    pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", quad_fragment_shader);
    pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_->SetInputStreams(&quad_);
    pipeline_->SetViewport(vulkanInfo.viewport);
    pipeline_->SetScissor(vulkanInfo.scissor);
    pipeline_->SetSamples(vulkanInfo.num_samples);
    pipeline_->AddAttachment();

    pipeline_->Commit();

    color_data_ = containers::make_unique<vulkan::BufferFrameData<Data>>(
        allocator, app, num_swapchain_images, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    populateData(app->GetLogger(), color_data_->data().data.data(),
                 color_data_->data().data.size(), VK_FORMAT_R8G8B8A8_UINT,
                 app->swapchain().format());
}

void PreQuad::InitializeInputImages(vulkan::VulkanApplication* app,
    PreQuadData* renderData,
    containers::Allocator* allocator) {
    // Create the staging image
    VkImageCreateInfo img_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UINT,          // format
        {
            app->swapchain().width(),   // width
            app->swapchain().height(),  // height
            app->swapchain().depth()    // depth,
        },
        1,                        // mipLevels
        1,                        // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,    // samples
        VK_IMAGE_TILING_OPTIMAL,  // tiling
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };

    renderData->color_staging_img_ = app->CreateAndBindImage(&img_info);

    // Create image views
    // color input view
    VkImageViewCreateInfo view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *renderData->color_staging_img_,           // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        renderData->color_staging_img_->format(),  // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_color_input_view;
    LOG_ASSERT(
        ==, app->GetLogger(), VK_SUCCESS,
        app->device()->vkCreateImageView(app->device(), &view_info, nullptr,
                                           &raw_color_input_view));
    renderData->color_input_view_ =
        containers::make_unique<vulkan::VkImageView>(allocator,
            vulkan::VkImageView(raw_color_input_view, nullptr, &app->device()));
}

void PreQuad::InitializeFrameData(vulkan::VulkanApplication* app,
    PreQuadData* renderData,
    containers::Allocator* allocator,
    const VkImageView& colorView,
    size_t frame_index) {

    InitializeInputImages(app, renderData, allocator);

    // Create a framebuffer for rendering
    VkImageView views[2] = {
        colorView,
        *renderData->color_input_view_,
    };

    // Create a framebuffer with attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        2,                                          // attachmentCount
        views,                                      // attachments
        app->swapchain().width(),                   // width
        app->swapchain().height(),                  // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app->device()->vkCreateFramebuffer(
        app->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    renderData->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        allocator,
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app->device()));

    renderData->descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(allocator,
            app->AllocateDescriptorSet({descriptor_set_layout_binding_}));

    VkDescriptorImageInfo input_attachment_info = {
        VK_NULL_HANDLE,                            // sampler
        *renderData->color_input_view_,            // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };
    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *renderData->descriptor_set_,            // dstSet
        0,                                       // dstBinding
        0,                                       // dstArrayElement
        1,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,     // descriptorType
        &input_attachment_info,                  // pImageInfo
        nullptr,                                 // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    app->device()->vkUpdateDescriptorSets(
        app->device(), 1, &write, 0, nullptr);
}

void PreQuad::CopyInputImages(vulkan::VulkanApplication* app, PreQuadData* renderData, vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index) {
    // Copy data from color source buffer to the staging images.
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
        *renderData->color_staging_img_,         // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_HOST_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_TRANSFER_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_TRANSFER_BIT,      // dstStageMask
            0,                                   // dependencyFlags
            0,                                   // memoryBarrierCount
            nullptr,                             // memoryBarrierCount
            1,                                   // bufferMemoryBarrierCount
            &color_data_to_src,                  // pBufferMemoryBarriers
            1,                                   // imageMemoryBarrierCount
            &color_input_undef_to_dst            // pImageMemoryBarriers
        );

    LOG_ASSERT(>=, app->GetLogger(), app->swapchain().width(), src_data.width);
    LOG_ASSERT(>=, app->GetLogger(), app->swapchain().height(), src_data.height);
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
    cmdBuffer->vkCmdCopyBufferToImage(cmdBuffer, color_data_->get_buffer(),
            *renderData->color_staging_img_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

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
        *renderData->color_staging_img_,           // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    cmdBuffer->vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,         // srcStageMask
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  // dstStageMask
            0,                                      // dependencyFlags
            0,                                      // memoryBarrierCount
            nullptr,                                // memoryBarrierCount
            0,                                      // bufferMemoryBarrierCount
            nullptr,                                // pBufferMemoryBarriers
            1,                                      // imageMemoryBarrierCount
            &color_input_dst_to_shader_read         // pImageMemoryBarriers
        );
}

void PreQuad::RecordRenderCmds(vulkan::VulkanApplication* app, PreQuadData* renderData,
    vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index) {
    CopyInputImages(app, renderData, cmdBuffer, frame_index);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);
    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *(renderData->framebuffer_),               // framebuffer
        {{0, 0},
         {app->swapchain().width(),
          app->swapchain().height()}},    // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *pipeline_);

    cmdBuffer->vkCmdBindDescriptorSets(cmdBuffer,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  ::VkPipelineLayout(*pipeline_layout_), 0, 1,
                                  &renderData->descriptor_set_->raw_set(), 0,
                                  nullptr);
    quad_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);
}

void PreQuad::Update(float time_since_last_render) {
    // Do nothing
}

void PreQuad::UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index) {
    // Do nothing
    color_data_->UpdateBuffer(queue, frame_index);
}