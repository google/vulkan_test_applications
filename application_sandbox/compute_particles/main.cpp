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
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "particle_data_shared.h"
#include "support/containers/deque.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"
#include "vulkan_helpers/vulkan_texture.h"

namespace quad_model {
#include "fullscreen_quad.obj.h"
}
const auto& quad_data = quad_model::model;

uint32_t simulation_shader[] =
#include "particle_update.comp.spv"
    ;

uint32_t velocity_shader[] =
#include "particle_velocity_update.comp.spv"
    ;

uint32_t particle_fragment_shader[] =
#include "particle.frag.spv"
    ;

uint32_t particle_vertex_shader[] =
#include "particle.vert.spv"
    ;

namespace particle_texture {
#include "particle.png.h"
}

const auto& texture_data = particle_texture::texture;

struct time_data {
  int32_t frame_number;
  float time;
};

class ComputeTask {
 public:
  ComputeTask(containers::Allocator* allocator, vulkan::VulkanApplication* app)
      : allocator_(allocator),
        compute_data_(allocator),
        app_(app),
        last_update_time_(std::chrono::high_resolution_clock::now()) {
    if (!app_->async_compute_queue()) {
      return;
    }

    update_time_data_ = containers::make_unique<vulkan::BufferFrameData<Mat44>>(
        allocator_, app_, app_->swapchain_images().size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0,
        app_->async_compute_queue()->index());

    InitSimulationSSBO();
    InitRenderSSBO();
    CreateComputePipelines();
    InitComputeTaskData();
  }

  vulkan::VulkanApplication::Buffer* GetBufferForRender() const {
    return render_ssbo_.get();
  }

  vulkan::VkSemaphore* GetSemaphoreForIndex(int32_t buffer) const {
    return compute_data_[buffer].semaphore_.get();
  }

  void SubmitComputeTask(uint32_t frame_index, VkSemaphore waitSemaphores) {
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_time =
        current_time - last_update_time_;
    last_update_time_ = current_time;

    std::chrono::duration<float> time_since_last_notify =
        current_time - last_notify_time_;
    if (time_since_last_notify.count() > 1.0f) {
      app_->GetLogger()->LogInfo("Simulated ", simulation_count_, " steps in ",
                                 time_since_last_notify.count(), "s.");
      last_notify_time_ = current_time;
      simulation_count_ = 0;
    }
    simulation_count_++;

    update_time_data_->data()[0] = static_cast<float>(current_frame++);
    update_time_data_->data()[1] = elapsed_time.count();
    if (current_frame >= TOTAL_PARTICLES) {
      current_frame = 0;
    }
    update_time_data_->UpdateBuffer(app_->async_compute_queue(), frame_index);

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    auto& dat = compute_data_[frame_index];
    // This is where the computation actually happens
    VkSubmitInfo computation_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        1,                              // waitSemaphoreCount
        &waitSemaphores,                // pWaitSemaphores
        &waitStageMask,                 // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(dat.command_buffer_.get_command_buffer()),
        1,  // signalSemaphoreCount
        &GetSemaphoreForIndex(frame_index)
             ->get_raw_object()  // pSignalSemaphores
    };

    (*app_->async_compute_queue())
        ->vkQueueSubmit(*app_->async_compute_queue(), 1,
                        &computation_submit_info,
                        static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  void InitSimulationSSBO() {
    auto initial_data_buffer = containers::make_unique<vulkan::VkCommandBuffer>(
        allocator_, GetComputeCommandBuffer());

    (*initial_data_buffer)
        ->vkBeginCommandBuffer(*initial_data_buffer,
                               &sample_application::kBeginCommandBuffer);

    // Create the single SSBO for simulation
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,       // sType
        nullptr,                                    // pNext
        0,                                          // createFlags
        sizeof(simulation_data) * TOTAL_PARTICLES,  // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,  // usageFlags
        VK_SHARING_MODE_EXCLUSIVE,               // sharingMode
        0,                                       // queueFamilyIndexCount
        nullptr                                  // pQueueFamilyIndices
    };

    simulation_ssbo_ = app_->CreateAndBindDeviceBuffer(&create_info);

    srand(0);
    // Fill this SSBO with random initial positions.
    containers::vector<simulation_data> fill_data(allocator_);
    fill_data.resize(TOTAL_PARTICLES);
    for (auto& particle : fill_data) {
      float distance =
          static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      angle = angle * 3.1415f * 2.0f;
      float x = sin(angle);
      float y = cos(angle);

      particle.position_velocity[0] = x * (1 - (distance * distance));
      particle.position_velocity[1] = y * (1 - (distance * distance));
      float posx = particle.position_velocity[0];
      float posy = particle.position_velocity[1];
      particle.position_velocity[2] = -posy * 0.05f;
      particle.position_velocity[3] = posx * 0.05f;
    }

    // Fill the buffer. Technically we probably want to use a staging buffer
    // and fill from that, since this is not really a "small" buffer.
    // However, we have this helper function, so might as well use it.
    app_->FillSmallBuffer(
        simulation_ssbo_.get(), fill_data.data(),
        fill_data.size() * sizeof(simulation_data), 0,
        initial_data_buffer.get(),
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    (*initial_data_buffer)->vkEndCommandBuffer(*initial_data_buffer);

    VkSubmitInfo setup_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(initial_data_buffer->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    // Actually finish filling the initial data, and transfer to the
    // GPU.
    (*app_->async_compute_queue())
        ->vkQueueSubmit(*app_->async_compute_queue(), 1, &setup_submit_info,
                        ::VkFence(VK_NULL_HANDLE));

    // Wait for it all to be done.
    (*app_->async_compute_queue())
        ->vkQueueWaitIdle((*app_->async_compute_queue()));
  }

  void InitComputeTaskData() {
    // For each async compute buffer, we have to create the semaphore,
    // the command_buffers, descriptor sets, and some synchronization data.
    for (size_t i = 0; i < app_->swapchain_images().size(); ++i) {
      compute_data_.push_back(ComputeTaskData{
          containers::make_unique<vulkan::VkSemaphore>(
              allocator_, vulkan::CreateSemaphore(&app_->device())),
          GetComputeCommandBuffer(),
          containers::make_unique<vulkan::DescriptorSet>(
              allocator_, app_->AllocateDescriptorSet(
                              {compute_descriptor_set_layouts_[0],
                               compute_descriptor_set_layouts_[1],
                               compute_descriptor_set_layouts_[2]}))});

      VkDescriptorBufferInfo buffer_infos[3] = {
          {
              update_time_data_->get_buffer(),             // buffer
              update_time_data_->get_offset_for_frame(i),  // offset
              update_time_data_->size(),                   // range
          },
          {
              *simulation_ssbo_,         // buffer
              0,                         // offset
              simulation_ssbo_->size(),  // range
          },
          {
              *render_ssbo_,         // buffer
              0,                     // offset
              render_ssbo_->size(),  // range
          },
      };
      VkWriteDescriptorSet write = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,         // sType
          nullptr,                                        // pNext
          *compute_data_.back().compute_descriptor_set_,  // dstSet
          0,                                              // dstbinding
          0,                                              // dstArrayElement
          3,                                              // descriptorCount
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,              // descriptorType
          nullptr,                                        // pImageInfo
          buffer_infos,                                   // pBufferInfo
          nullptr,                                        // pTexelBufferView
      };

      app_->device()->vkUpdateDescriptorSets(app_->device(), 1, &write, 0,
                                             nullptr);

      auto& dat = compute_data_.back();
      VkBufferMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
          nullptr,                                  // pNext
          0,                                        // srcAccessMask
          0,                                        // dstAccessMask
          app_->render_queue().index(),             // srcQueueFamilyIndex
          app_->async_compute_queue()->index(),     // dstQueueFamilyIndex
          *render_ssbo_,                            // buffer
          0,                                        //  offset
          render_ssbo_->size(),                     // size
      };

      auto& command_buffer = dat.command_buffer_;
      command_buffer->vkBeginCommandBuffer(
          command_buffer, &sample_application::kBeginCommandBuffer);

      // Transfer the ownership from the render_queue to this queue.
      command_buffer->vkCmdPipelineBarrier(
          command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0,
          nullptr);

      command_buffer->vkCmdBindDescriptorSets(
          command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          ::VkPipelineLayout(*compute_pipeline_layout_), 0, 1,
          &dat.compute_descriptor_set_->raw_set(), 0, nullptr);
      command_buffer->vkCmdBindPipeline(
          command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, *velocity_pipeline_);
      // Run the first half of the simulation.
      command_buffer->vkCmdDispatch(
          command_buffer, TOTAL_PARTICLES / COMPUTE_SHADER_LOCAL_SIZE, 1, 1);
      VkBufferMemoryBarrier simulation_barrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
          nullptr,                                  // pNext
          VK_ACCESS_SHADER_WRITE_BIT,               // srcAccessMask
          VK_ACCESS_SHADER_READ_BIT,                // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
          *simulation_ssbo_,                        // buffer
          0,                                        //  offset
          simulation_ssbo_->size(),                 // size
      };
      // Wait for all of the updates to velocity to be done before
      // moving on to the position updates. This is because the velocity
      // for a single particle is dependent on the positions of all other
      // particles, so avoid race conditions.
      command_buffer->vkCmdPipelineBarrier(
          command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
          &simulation_barrier, 0, nullptr);
      command_buffer->vkCmdBindPipeline(command_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        *position_update_pipeline_);
      // Update the positions, and fill the output buffer.
      command_buffer->vkCmdDispatch(
          command_buffer, TOTAL_PARTICLES / COMPUTE_SHADER_LOCAL_SIZE, 1, 1);

      // Transition the old buffer back.
      barrier.srcQueueFamilyIndex = app_->async_compute_queue()->index();
      barrier.dstQueueFamilyIndex = app_->render_queue().index();
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = 0;

      command_buffer->vkCmdPipelineBarrier(
          command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0,
          nullptr);

      command_buffer->vkEndCommandBuffer(command_buffer);
    }
  }

  void CreateComputePipelines() {
    // Both compute passes use the same set of descriptors for simplicity.
    // Technically we don't have to pass the draw_data SSBO to the velocity
    // update shader, but we don't want to have to do twice the work.
    compute_descriptor_set_layouts_[0] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
        nullptr                             // pImmutableSamplers
    };
    compute_descriptor_set_layouts_[1] = {
        2,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
        nullptr                             // pImmutableSamplers
    };
    // This should ideally be a UBO, but I was getting hangs in the shader
    // when using it as a UBO, switching to an SSBO worked.
    compute_descriptor_set_layouts_[2] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
        nullptr                             // pImmutableSamplers
    };

    // This is the pipeline that updates the position, and transfers
    // the data to the other thread.
    compute_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator_,
        app_->CreatePipelineLayout({{compute_descriptor_set_layouts_[0],
                                     compute_descriptor_set_layouts_[1],
                                     compute_descriptor_set_layouts_[2]}}));
    position_update_pipeline_ =
        containers::make_unique<vulkan::VulkanComputePipeline>(
            allocator_,
            app_->CreateComputePipeline(
                compute_pipeline_layout_.get(),
                VkShaderModuleCreateInfo{
                    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                    sizeof(simulation_shader), simulation_shader},
                "main"));

    // This is the pipeline that updates the velocity based on all of the
    // particles positions.
    velocity_pipeline_ = containers::make_unique<vulkan::VulkanComputePipeline>(
        allocator_,
        app_->CreateComputePipeline(
            compute_pipeline_layout_.get(),
            VkShaderModuleCreateInfo{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                sizeof(velocity_shader), velocity_shader},
            "main"));
  }

  void InitRenderSSBO() {
    uint32_t queue_family_indices[2] = {app_->render_queue().index(),
                                        app_->async_compute_queue()->index()};
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // createFlags
        sizeof(draw_data) * TOTAL_PARTICLES,   // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,  // usageFlags
        VK_SHARING_MODE_EXCLUSIVE,               // sharingMode
        2,                                       // queueFamilyIndexCount
        queue_family_indices                     // pQueueFamilyIndices
    };

    render_ssbo_ = app_->CreateAndBindDeviceBuffer(&create_info);
    app_->GetLogger()->LogInfo("render_ssbo_ buffer : ",
                               VkBuffer(*render_ssbo_));
  }

  vulkan::VkCommandBuffer GetComputeCommandBuffer() {
    return app_->GetCommandBuffer(app_->async_compute_queue()->index());
  }

  struct ComputeTaskData {
    containers::unique_ptr<vulkan::VkSemaphore> semaphore_;
    // The command buffer for simulating.
    vulkan::VkCommandBuffer command_buffer_;
    // The descriptor set needed for simulating.
    containers::unique_ptr<vulkan::DescriptorSet> compute_descriptor_set_;
  };

  containers::Allocator* allocator_;
  // The actual data associated with those buffers.
  containers::vector<ComputeTaskData> compute_data_;
  // This SSBO contains all of the up-to-date simulation information.
  // It is shared by all frames, since all frames need the most up-to-date
  // data.
  containers::unique_ptr<vulkan::VulkanApplication::Buffer> simulation_ssbo_;
  // The SSBO used for actually rendering.
  containers::unique_ptr<vulkan::VulkanApplication::Buffer> render_ssbo_;

  // This pipeline layout is shared between both velocity_pipeline_ and
  // position_update_pipeline_.
  containers::unique_ptr<vulkan::PipelineLayout> compute_pipeline_layout_;
  // These descriptor sets are shared by both pipelines as well.
  VkDescriptorSetLayoutBinding compute_descriptor_set_layouts_[3];
  // This pipeline is used to update the velocity component of the
  // simulation_ssbo_.
  containers::unique_ptr<vulkan::VulkanComputePipeline> velocity_pipeline_;
  // This pipeline is used to update the position of every element in the
  // simulation_ssbo_.
  containers::unique_ptr<vulkan::VulkanComputePipeline>
      position_update_pipeline_;

  // This contains the current timing information.
  containers::unique_ptr<vulkan::BufferFrameData<Mat44>> update_time_data_;
  // The time that the last update was started.
  std::chrono::time_point<std::chrono::high_resolution_clock> last_update_time_;

  int current_frame = 0;

  // The number of times the simulation has run since the last log.
  uint32_t simulation_count_ = 0;

  std::chrono::time_point<std::chrono::high_resolution_clock> last_notify_time_;

  vulkan::VulkanApplication* app_;
};

struct ComputeParticlesFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> draw_command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> particle_descriptor_set_;
  containers::unique_ptr<vulkan::VkSemaphore> render_semaphore_;
};

class ComputeParticlesSample
    : public sample_application::Sample<ComputeParticlesFrameData> {
 public:
  ComputeParticlesSample(const entry::EntryData* data)
      : data_(data),
        Sample<ComputeParticlesFrameData>(data->allocator(), data, 1, 512, 32,
                                          1,
                                          sample_application::SampleOptions()
                                              .EnableAsyncCompute()
                                              .EnableMultisampling()),
        quad_model_(data->allocator(), data->logger(), quad_data),
        particle_texture_(data->allocator(), data->logger(), texture_data),
        compute_task_(data->allocator(), app()) {
    if (!app()->async_compute_queue()) {
      app()->GetLogger()->LogError("Could not find async compute queue.");
      set_invalid(true);
    }
  }

  void prepareDrawPipeline() {
    particle_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    particle_descriptor_set_layouts_[3] = {
        3,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    particle_descriptor_set_layouts_[1] = {
        1,                             // binding
        VK_DESCRIPTOR_TYPE_SAMPLER,    // descriptorType
        1,                             // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,  // stageFlags
        nullptr                        // pImmutableSamplers
    };
    particle_descriptor_set_layouts_[2] = {
        2,                                 // binding
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr                            // pImmutableSamplers
    };

    sampler_ = containers::make_unique<vulkan::VkSampler>(
        data_->allocator(),
        vulkan::CreateSampler(&app()->device(), VK_FILTER_LINEAR,
                              VK_FILTER_LINEAR));

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{particle_descriptor_set_layouts_[0],
                                      particle_descriptor_set_layouts_[1],
                                      particle_descriptor_set_layouts_[2],
                                      particle_descriptor_set_layouts_[3]}}));

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

    particle_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    particle_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                  particle_vertex_shader);
    particle_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                  particle_fragment_shader);
    particle_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    particle_pipeline_->SetInputStreams(&quad_model_);
    particle_pipeline_->SetViewport(viewport());
    particle_pipeline_->SetScissor(scissor());
    particle_pipeline_->SetSamples(num_samples());
    particle_pipeline_->AddAttachment(VkPipelineColorBlendAttachmentState{
        VK_TRUE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE,
        VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});
    particle_pipeline_->Commit();
  }

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    aspect_buffer_ = containers::make_unique<vulkan::BufferFrameData<Vector4>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    // All of this is the fairly standard setup for rendering.
    quad_model_.InitializeData(app(), initialization_buffer);
    particle_texture_.InitializeData(app(), initialization_buffer);
    prepareDrawPipeline();
  }

  virtual void InitializeFrameData(
      ComputeParticlesFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // All of this is the fairly standard setup for rendering.
    // The main difference here is that we re-create the command-buffers
    // every frame since we do not know which SSBO we will be rendering
    // out of for any given frame_index.

    frame_data->draw_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->particle_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {particle_descriptor_set_layouts_[0],
                                     particle_descriptor_set_layouts_[1],
                                     particle_descriptor_set_layouts_[2],
                                     particle_descriptor_set_layouts_[3]}));

    frame_data->render_semaphore_ =
        containers::make_unique<vulkan::VkSemaphore>(
            data_->allocator(), vulkan::CreateSemaphore(&app()->device()));
    ::VkImageView raw_view = color_view(frame_data);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        0,                              // commandBufferCount
        nullptr,
        1,  // signalSemaphoreCount
        &frame_data->render_semaphore_->get_raw_object()  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));

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

  virtual void InitializationComplete() override {
    particle_texture_.InitializationComplete();
  }

  virtual void Update(float delta_time) override {
    time_since_last_notify_ += delta_time;
    frames_since_last_notify_ += 1;
    if (time_since_last_notify_ > 1.0f) {
      app()->GetLogger()->LogInfo("Rendered ", frames_since_last_notify_,
                                  " frames in ", time_since_last_notify_, "s.");
      frames_since_last_notify_ = 0;
      time_since_last_notify_ = 0;
    }
    aspect_buffer_->data()[0] =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      ComputeParticlesFrameData* data) override {
    compute_task_.SubmitComputeTask(frame_index,
                                    data->render_semaphore_->get_raw_object());
    // Get the next buffer that we use for the particle positions.
    auto* buffer = compute_task_.GetBufferForRender();

    aspect_buffer_->UpdateBuffer(&app()->render_queue(), frame_index);

    // Write that buffer into the descriptor sets.
    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            *buffer,         // buffer
            0,               // offset
            buffer->size(),  // range
        },
        {
            aspect_buffer_->get_buffer(),                       // buffer
            aspect_buffer_->get_offset_for_frame(frame_index),  // offset
            aspect_buffer_->size(),                             // range
        }};

    VkDescriptorImageInfo sampler_info = {
        *sampler_,                 // sampler
        VK_NULL_HANDLE,            // imageView
        VK_IMAGE_LAYOUT_UNDEFINED  //  imageLayout
    };
    VkDescriptorImageInfo texture_info = {
        VK_NULL_HANDLE,                            // sampler
        particle_texture_.view(),                  // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };

    VkWriteDescriptorSet writes[4]{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *data->particle_descriptor_set_,         // dstSet
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
            *data->particle_descriptor_set_,         // dstSet
            3,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            &buffer_infos[1],                        // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *data->particle_descriptor_set_,         // dstSet
            1,                                       // dstbinding
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
            *data->particle_descriptor_set_,         // dstSet
            2,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        // descriptorType
            &texture_info,                           // pImageInfo
            nullptr,                                 // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 4, writes, 0,
                                            nullptr);

    // Record our command-buffer for rendering this frame
    (*data->draw_command_buffer_)
        ->vkResetCommandBuffer((*data->draw_command_buffer_), 0);
    (*data->draw_command_buffer_)
        ->vkBeginCommandBuffer((*data->draw_command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*data->draw_command_buffer_);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);
    clear.color.float32[3] = 1.0f;

    VkBufferMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        0,                                        // srcAccessMask
        0,                                        //  dstAccessMask
        app()->async_compute_queue()->index(),    // srcQueueFamilyIndex
        app()->render_queue().index(),            // dstQueueFamilyIndex
        *buffer,                                  // bufferdraw_data
        0,                                        //  offset
        buffer->size(),                           // size
    };
    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
                                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                    nullptr, 1, &barrier, 0, nullptr);
    // }

    // The rest of the normal drawing.
    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *data->framebuffer_,                       // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *particle_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &data->particle_descriptor_set_->raw_set(), 0, nullptr);
    // We only have to draw one model N times, in the shader we move
    // each instance to the correct location.
    quad_model_.DrawInstanced(&cmdBuffer, TOTAL_PARTICLES);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    VkBufferMemoryBarrier transfer_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        0,                                        // srcAccessMask
        0,                                        //  dstAccessMask
        app()->render_queue().index(),            // srcQueueFamilyIndex
        app()->async_compute_queue()->index(),    // dstQueueFamilyIndex
        *buffer,                                  // buffer
        0,                                        //  offset
        buffer->size(),                           // size
    };
    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                    nullptr, 1, &transfer_barrier, 0, nullptr);

    (*data->draw_command_buffer_)
        ->vkEndCommandBuffer(*data->draw_command_buffer_);

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        1,                              // waitSemaphoreCount
        &compute_task_.GetSemaphoreForIndex(frame_index)
             ->get_raw_object(),  // pWaitSemaphores
        &waitStageMask,           // pWaitDstStageMask,
        1,                        // commandBufferCount
        &(data->draw_command_buffer_->get_command_buffer()),
        1,                                          // signalSemaphoreCount
        &data->render_semaphore_->get_raw_object()  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  const entry::EntryData* data_;

  // All of the data needed for the particle rendering pipeline.
  VkDescriptorSetLayoutBinding particle_descriptor_set_layouts_[4];
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> particle_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;

  // This ssbo just contains the aspect ratio. We use a Vector4 just so we
  // get proper alignment.
  containers::unique_ptr<vulkan::BufferFrameData<Vector4>> aspect_buffer_;
  // A model of a quad with corners. (-1, -1), (1, 1), (-1, 1), (1, -1)
  vulkan::VulkanModel quad_model_;
  // A simple circular texture with falloff.
  vulkan::VulkanTexture particle_texture_;
  // The sampler for this texture.
  containers::unique_ptr<vulkan::VkSampler> sampler_;
  // Data so that we can print out update information once per frame.
  float time_since_last_notify_ = 0.f;
  uint32_t frames_since_last_notify_ = 0;
  ComputeTask compute_task_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  ComputeParticlesSample sample(data);
  if (!sample.is_valid()) {
    data->logger()->LogInfo("Application is invalid.");
    return -1;
  }
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
