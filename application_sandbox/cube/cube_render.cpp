#include "cube_render.h"

namespace cube_model {
#include "cube.obj.h"
}

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

    uint32_t cube_vertex_shader[] =
    #include "cube.vert.spv"
        ;

    uint32_t cube_fragment_shader[] =
    #include "cube.frag.spv"
        ;
    const auto& cube_data = cube_model::model;
}

CubeRender::CubeRender(const entry::EntryData* data) 
    : cube_(data->allocator(), data->logger(), cube_data),
    quad_(data->allocator(), data->logger(), quad_data) {}

void CubeRender::InitializeCubeData(vulkan::VulkanApplication* app,
    containers::Allocator* allocator,
    CubeVulkanInfo vulkanInfo,
    vulkan::VkCommandBuffer* initialization_buffer,  
    size_t num_swapchain_images) {
    quad_.InitializeData(app, initialization_buffer);
    cube_.InitializeData(app, initialization_buffer);

    first_pass_descriptor_set_layout = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // desriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

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

    first_pass_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator,
        app->CreatePipelineLayout(
            {{
                first_pass_descriptor_set_layout,
            }})
    );

    cube_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator,
        app->CreatePipelineLayout(
            {{
                descriptor_set_layouts_[0],
                descriptor_set_layouts_[1],
            }})
    );

    VkAttachmentReference input_attachment = {
        0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_stencil_attachment = {
        2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription input_attachment_description = {
        0,                                        // flags
        vulkanInfo.colorFormat,                   // format
        vulkanInfo.num_samples,                   // samples
        VK_ATTACHMENT_LOAD_OP_LOAD,               // loadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // finalLayout
    };

    VkAttachmentDescription color_attachment_description = {
        0,                                        // flags
        vulkanInfo.colorFormat,                   // format
        vulkanInfo.num_samples,                   // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // initialLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout
    };

    VkAttachmentDescription depth_attachment_description = {
        0,                                        // flags
        vulkanInfo.depthFormat,                   // format
        vulkanInfo.num_samples,                   // samples
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL  // finalLayout
    };

    VkSubpassDescription first_subpass_description = VkSubpassDescription {
        0,                                // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
        1,                                // inputAttachmentCount
        &input_attachment,                 // pInputAttachments
        1,                                // colorAttachmentCount
        &color_attachment,                // colorAttachment
        nullptr,                          // pResolveAttachments
        nullptr,                          // pDepthStencilAttachment
        0,                                // preserveAttachmentCount
        nullptr                           // pPreserveAttachments
    };

    VkSubpassDescription second_subpass_description = VkSubpassDescription {
        0,                                // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
        0,                                // inputAttachmentCount
        nullptr,                          // pInputAttachments
        1,                                // colorAttachmentCount
        &color_attachment,                // colorAttachment
        nullptr,                          // pResolveAttachments
        &depth_stencil_attachment,        // pDepthStencilAttachment
        0,                                // preserveAttachmentCount
        nullptr                           // pPreserveAttachments
    };

    VkSubpassDependency first_subpass_dependency = VkSubpassDependency {
        0,
        1,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        1
    };

    VkSubpassDependency second_subpass_dependency = VkSubpassDependency {
        0,
        1,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        1
    };

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        allocator,
        app->CreateRenderPass(
            {
                input_attachment_description,
                color_attachment_description,
                depth_attachment_description
            },  // AttachmentDescriptions
            {
                first_subpass_description,
                second_subpass_description
            }, // SubpassDescriptions
            {
                first_subpass_dependency,
                second_subpass_dependency,
            } // SubpassDependencies
        )
    );

    first_pass_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        allocator,
        app->CreateGraphicsPipeline(first_pass_pipeline_layout_.get(), render_pass_.get(), 0));
    first_pass_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", quad_vertex_shader);
    first_pass_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", quad_fragment_shader);
    first_pass_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    first_pass_pipeline_->SetInputStreams(&quad_);
    first_pass_pipeline_->SetViewport(vulkanInfo.viewport);
    first_pass_pipeline_->SetScissor(vulkanInfo.scissor);
    first_pass_pipeline_->SetSamples(vulkanInfo.num_samples);
    first_pass_pipeline_->AddAttachment();
    first_pass_pipeline_->Commit();

    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        allocator,
        app->CreateGraphicsPipeline(cube_pipeline_layout_.get(), render_pass_.get(), 1));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", cube_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", cube_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(vulkanInfo.viewport);
    cube_pipeline_->SetScissor(vulkanInfo.scissor);
    cube_pipeline_->SetSamples(vulkanInfo.num_samples);
    cube_pipeline_->DepthStencilState().depthCompareOp = VK_COMPARE_OP_ALWAYS;
    cube_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        allocator, app, num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        allocator, app, num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app->swapchain().width() / (float)app->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat4x4::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat4x4::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform = Mat4x4::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
}

void CubeRender::InitializeFrameData(vulkan::VulkanApplication* app,
    CubeRenderData* renderData,
    containers::Allocator* allocator,
    const VkImageView& inputView,
    const VkImageView& colorView,
    const VkImageView& depthView,
    size_t frame_index) {

    renderData->first_pass_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            allocator,
            app->AllocateDescriptorSet({first_pass_descriptor_set_layout}));

    VkDescriptorImageInfo input_attachment_info = {
        VK_NULL_HANDLE,                            // sampler
        inputView,                                 // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };
    VkWriteDescriptorSet first_pass_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *renderData->first_pass_descriptor_set_, // dstSet
        0,                                       // dstBinding
        0,                                       // dstArrayElement
        1,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,     // descriptorType
        &input_attachment_info,                  // pImageInfo
        nullptr,                                 // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    app->device()->vkUpdateDescriptorSets(app->device(), 1, &first_pass_write, 0, nullptr);

    renderData->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            allocator,
            app->AllocateDescriptorSet({descriptor_set_layouts_[0],
                                          descriptor_set_layouts_[1]}));

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

    VkWriteDescriptorSet cube_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *(renderData->cube_descriptor_set_),     // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app->device()->vkUpdateDescriptorSets(app->device(), 1, &cube_write, 0,
                                            nullptr);

    const int attachment_count = 3;
    // Create a framebuffer for rendering
    VkImageView views[attachment_count] = {
        inputView,
        colorView,
        depthView,
    };

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        attachment_count,                           // attachmentCount
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
}

void CubeRender::ClearDepthStencilAttachments(vulkan::VulkanApplication* app, vulkan::VkCommandBuffer& cmdBuffer) {
    VkClearValue clear_value = {
            0.0f,
            1
    };

    VkClearAttachment clear_attachment = {
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        1,
        clear_value,
    };

    VkClearRect rect = {
        {
            {
                0,
                0
            },
            {
                app->swapchain().width(),
                app->swapchain().height()
            }
        },
        0,
        1
    };

    cmdBuffer->vkCmdClearAttachments(cmdBuffer, 1, &clear_attachment, 1, &rect);
}

void CubeRender::ClearColorAttachments(vulkan::VulkanApplication* app, vulkan::VkCommandBuffer& cmdBuffer) {
    VkClearValue clear_value = {
            0.0f, 0.0f, 0.0f, 0.0f
    };

    VkClearAttachment clear_attachment = {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        clear_value,
    };

    VkClearRect rect = {
        {
            {
                0,
                0
            },
            {
                app->swapchain().width(),
                app->swapchain().height()
            }
        },
        0,
        1
    };

    cmdBuffer->vkCmdClearAttachments(cmdBuffer, 1, &clear_attachment, 1, &rect);
}

void CubeRender::RecordRenderCmds(vulkan::VulkanApplication* app,
    CubeRenderData* renderData, vulkan::VkCommandBuffer& cmdBuffer) {
    VkClearValue clear;
    vulkan::MemoryClear(&clear);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *(renderData->framebuffer_),                 // framebuffer
        {{0, 0},
         {app->swapchain().width(),
          app->swapchain().height()}},  // renderArea
        1,                              // clearValueCount
        &clear                          // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*first_pass_pipeline_layout_),
        0,
        1,
        &(renderData->first_pass_descriptor_set_->raw_set()), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *first_pass_pipeline_);
    quad_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*cube_pipeline_layout_),
        0,
        1,
        &(renderData->cube_descriptor_set_->raw_set()), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *cube_pipeline_);
    // ClearColorAttachments(app, cmdBuffer);
    cube_.Draw(&cmdBuffer);
    ClearDepthStencilAttachments(app, cmdBuffer);
    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);
}

void CubeRender::Update(float time_since_last_render) {
    model_data_->data().transform = model_data_->data().transform *
        Mat4x4::FromRotationMatrix(
            Mat4x4::RotationX(3.14f * time_since_last_render) *
            Mat4x4::RotationY(3.14f * time_since_last_render * 0.5f));
}

void CubeRender::UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index) {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);
}