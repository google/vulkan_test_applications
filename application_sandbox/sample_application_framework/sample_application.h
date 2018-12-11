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

#ifndef SAMPLE_APPLICATION_FRAMEWORK_SAMPLE_APPLICATION_H_
#define SAMPLE_APPLICATION_FRAMEWORK_SAMPLE_APPLICATION_H_

#include "support/entry/entry.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"

#include <chrono>
#include <cstddef>

namespace sample_application {

const static VkSampleCountFlagBits kVkMultiSampledSampleCount =
    VK_SAMPLE_COUNT_4_BIT;
const static VkFormat kDepthFormat = VK_FORMAT_D16_UNORM;

struct SampleOptions {
  bool enable_multisampling = false;
  bool enable_depth_buffer = false;
  bool verbose_output = false;
  bool async_compute = false;
  bool sparse_binding = false;

  SampleOptions& EnableMultisampling() {
    enable_multisampling = true;
    return *this;
  }
  SampleOptions& EnableDepthBuffer() {
    enable_depth_buffer = true;
    return *this;
  }
  SampleOptions& EnableVerbose() {
    verbose_output = true;
    return *this;
  }
  SampleOptions& EnableAsyncCompute() {
    async_compute = true;
    return *this;
  }
  SampleOptions& EnableSparseBinding() {
    sparse_binding = true;
    return *this;
  }
};

const VkCommandBufferBeginInfo kBeginCommandBuffer = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
    nullptr,                                      // pNext
    0,                                            // flags
    nullptr                                       // pInheritanceInfo
};

const VkCommandBufferInheritanceInfo kInheritanceCommandBuffer = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,  // sType
    nullptr,                                            // pNext
    ::VkRenderPass(VK_NULL_HANDLE),                     // renderPass
    0,                                                  // subPass
    ::VkFramebuffer(VK_NULL_HANDLE),                    // framebuffer
    VK_FALSE,                                           // occlusionQueryEnable
    0,                                                  // queryFlags
};

const VkSubmitInfo kEmptySubmitInfo{
    VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
    nullptr,                        // pNext
    0,                              // waitSemaphoreCount
    nullptr,                        // pWaitSemaphores
    nullptr,                        // pWaitDstStageMask,
    0,                              // commandBufferCount
    nullptr,
    0,       // signalSemaphoreCount
    nullptr  // pSignalSemaphores
};

template <typename FrameData>
class Sample {
  // The per-frame data for an application.
  struct SampleFrameData {
    // The swapchain-image that this frame will use for rendering
    ::VkImage swapchain_image_;
    // The view for the image that is to be rendered to on this frame.
    containers::unique_ptr<vulkan::VkImageView> image_view;
    // The view for the depth that is to be rendered to on this frame
    containers::unique_ptr<vulkan::VkImageView> depth_view_;
    // A commandbuffer to transfer the swapchain from the present queue to
    // the main queue.
    containers::unique_ptr<vulkan::VkCommandBuffer>
        transfer_from_present_command_buffer_;
    // A commandbuffer to setup the rendering for this frame
    containers::unique_ptr<vulkan::VkCommandBuffer> setup_command_buffer_;
    // The commandbuffer that resolves the images and gets them ready for
    // present
    containers::unique_ptr<vulkan::VkCommandBuffer> resolve_command_buffer_;
    // The commandbuffer that transfers the images from the graphics
    // queue to the present queue.
    containers::unique_ptr<vulkan::VkCommandBuffer>
        transfer_from_graphics_command_buffer_;
    // The semaphore that handles transfering the swapchain image
    // between the present and render queues.
    containers::unique_ptr<vulkan::VkSemaphore> transfer_semaphore_;
    // The depth_stencil image, if it exists.
    vulkan::ImagePointer depth_stencil_;
    // The multisampled render target if it exists.
    vulkan::ImagePointer multisampled_target_;
    // The semaphore controlling access to the swapchain.
    containers::unique_ptr<vulkan::VkSemaphore> ready_semaphore_;
    // The fence that signals that the resources for this frame are free.
    containers::unique_ptr<vulkan::VkFence> ready_fence_;
    // The application-specific data for this frame.
    FrameData child_data_;
  };

 public:
  Sample(containers::Allocator* allocator, const entry::EntryData* entry_data,
         uint32_t host_buffer_size_in_MB, uint32_t image_memory_size_in_MB,
         uint32_t device_buffer_size_in_MB, uint32_t coherent_buffer_size_in_MB,
         const SampleOptions& options,
         const VkPhysicalDeviceFeatures& physical_device_features = {0},
         const std::initializer_list<const char*> extensions = {})
      : options_(options),
        data_(entry_data),
        allocator_(allocator),
        application_(allocator, entry_data->logger(), entry_data, extensions,
                     physical_device_features,
                     host_buffer_size_in_MB * 1024 * 1024,
                     image_memory_size_in_MB * 1024 * 1024,
                     device_buffer_size_in_MB * 1024 * 1024,
                     coherent_buffer_size_in_MB * 1024 * 1024,
                     options.async_compute, options.sparse_binding),
        frame_data_(allocator),
        swapchain_images_(application_.swapchain_images()),
        last_frame_time_(std::chrono::high_resolution_clock::now()),
        initialization_command_buffer_(application_.GetCommandBuffer()),
        average_frame_time_(0),
        is_valid_(true) {
    if (data_->fixed_timestep()) {
      app()->GetLogger()->LogInfo("Running with a fixed timestep of 0.1s");
    }

    frame_data_.reserve(swapchain_images_.size());
    // TODO: The image format used by the swapchain image may not suppport
    // multi-sampling. Fix this later by adding a vkCmdBlitImage command
    // after the vkCmdResolveImage.
    render_target_format_ = application_.swapchain().format();
    num_samples_ = options.enable_multisampling ? kVkMultiSampledSampleCount
                                                : VK_SAMPLE_COUNT_1_BIT;
    default_viewport_ = {0.0f,
                         0.0f,
                         static_cast<float>(application_.swapchain().width()),
                         static_cast<float>(application_.swapchain().height()),
                         0.0f,
                         1.0f};

    default_scissor_ = {
        {0, 0},
        {application_.swapchain().width(), application_.swapchain().height()}};
  }

  // This must be called before any other methods on this class. It initializes
  // all of the data for this application. It calls InitializeApplicationData
  // on the subclass, as well as InitializeLocalFrameData for every
  // image in the swapchain.
  void Initialize() {
    initialization_command_buffer_->vkBeginCommandBuffer(
        initialization_command_buffer_, &kBeginCommandBuffer);

    InitializeApplicationData(&initialization_command_buffer_,
                              swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); ++i) {
      frame_data_.push_back(SampleFrameData());
      InitializeLocalFrameData(&frame_data_.back(),
                               &initialization_command_buffer_, i);
    }

    initialization_command_buffer_->vkEndCommandBuffer(
        initialization_command_buffer_);

    VkSubmitInfo submit_info = kEmptySubmitInfo;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers =
        &(initialization_command_buffer_.get_command_buffer());

    vulkan::VkFence init_fence = vulkan::CreateFence(&application_.device());

    application_.render_queue()->vkQueueSubmit(application_.render_queue(), 1,
                                               &submit_info,
                                               init_fence.get_raw_object());
    application_.device()->vkWaitForFences(application_.device(), 1,
                                           &init_fence.get_raw_object(), false,
                                           0xFFFFFFFFFFFFFFFF);
    // Bit gross but submit all of the fences here
    for (auto& frame_data : frame_data_) {
      application_.render_queue()->vkQueueSubmit(
          application_.render_queue(), 0, nullptr, *frame_data.ready_fence_);
    }

    InitializationComplete();
  }

  void WaitIdle() { app()->device()->vkDeviceWaitIdle(app()->device()); }

  // The format that we are using to render. This will be either the swapchain
  // format if we are not rendering multi-sampled, or the multisampled image
  // format if we are rendering multi-sampled.
  VkFormat render_format() const { return render_target_format_; }

  VkFormat depth_format() const { return kDepthFormat; }

  // The number of samples that we are rendering with.
  VkSampleCountFlagBits num_samples() const { return num_samples_; }

  vulkan::VulkanApplication* app() { return &application_; }
  const vulkan::VulkanApplication* app() const { return &application_; }

  const VkViewport& viewport() const { return default_viewport_; }
  const VkRect2D& scissor() const { return default_scissor_; }

  // This calls both Update(time) and Render() for the subclass.
  // The update is meant to update all of the non-graphics state of the
  // application. Render() is used to actually process the commands
  // for rendering this particular frame.
  void ProcessFrame() {
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_time = current_time - last_frame_time_;
    last_frame_time_ = current_time;
    Update(data_->fixed_timestep() ? 0.1f : elapsed_time.count());

    // Smooth this out, so that it is more sensible.
    average_frame_time_ =
        elapsed_time.count() * 0.05f + average_frame_time_ * 0.95f;

    uint32_t image_idx;

    // This is a bit weird as we have to make new semaphores every frame, but
    // for now this will do. It will get cleaned up the next time
    // this image is used.
    vulkan::VkSemaphore temp_semaphore =
        vulkan::CreateSemaphore(&app()->device());

    LOG_ASSERT(==, app()->GetLogger(), VK_SUCCESS,
               app()->device()->vkAcquireNextImageKHR(
                   app()->device(), app()->swapchain(), 0xFFFFFFFFFFFFFFFF,
                   temp_semaphore.get_raw_object(),
                   static_cast<::VkFence>(VK_NULL_HANDLE), &image_idx));

    ::VkFence ready_fence = *frame_data_[image_idx].ready_fence_;

    LOG_ASSERT(
        ==, app()->GetLogger(), VK_SUCCESS,
        app()->device()->vkWaitForFences(app()->device(), 1, &ready_fence,
                                         VK_FALSE, 0xFFFFFFFFFFFFFFFF));
    LOG_ASSERT(
        ==, app()->GetLogger(), VK_SUCCESS,
        app()->device()->vkResetFences(app()->device(), 1, &ready_fence));
    if (options_.verbose_output) {
      app()->GetLogger()->LogInfo("Rendering frame <", elapsed_time.count(),
                                  ">: <", image_idx, ">", " Average: <",
                                  average_frame_time_, ">");
    }

    frame_data_[image_idx].ready_semaphore_ =
        containers::make_unique<vulkan::VkSemaphore>(allocator_,
                                                     std::move(temp_semaphore));
    ::VkSemaphore ready_semaphore = *frame_data_[image_idx].ready_semaphore_;

    ::VkSemaphore render_wait_semaphore = ready_semaphore;

    VkPipelineStageFlags flags =
        VkPipelineStageFlags(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    if (application_.HasSeparatePresentQueue()) {
      render_wait_semaphore = *frame_data_[image_idx].transfer_semaphore_;
      VkSubmitInfo transfer_submit_info{
          VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
          nullptr,                        // pNext
          1,                              // waitSemaphoreCount
          &ready_semaphore,               // pWaitSemaphores
          &flags,                         // pWaitDstStageMask,
          1,                              // commandBufferCount
          &(frame_data_[image_idx]
                .transfer_from_present_command_buffer_->get_command_buffer()),
          1,                      // signalSemaphoreCount
          &render_wait_semaphore  // pSignalSemaphores
      };

      app()->present_queue()->vkQueueSubmit(
          app()->present_queue(), 1, &transfer_submit_info,
          static_cast<::VkFence>(VK_NULL_HANDLE));
    }

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        1,                              // waitSemaphoreCount
        &render_wait_semaphore,         // pWaitSemaphores
        &flags,                         // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(frame_data_[image_idx].setup_command_buffer_->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    ::VkSemaphore present_ready_semaphore = render_wait_semaphore;
    if (application_.HasSeparatePresentQueue()) {
      present_ready_semaphore = *frame_data_[image_idx].transfer_semaphore_;
    }

    app()->render_queue()->vkQueueSubmit(
        app()->render_queue(), 1, &init_submit_info,
        static_cast<::VkFence>(VK_NULL_HANDLE));

    Render(&app()->render_queue(), image_idx,
           &frame_data_[image_idx].child_data_);
    init_submit_info.pCommandBuffers =
        &(frame_data_[image_idx].resolve_command_buffer_->get_command_buffer());

    init_submit_info.waitSemaphoreCount = 0;
    init_submit_info.pWaitSemaphores = nullptr;
    init_submit_info.pWaitDstStageMask = nullptr;
    init_submit_info.signalSemaphoreCount = 1;
    init_submit_info.pSignalSemaphores = &present_ready_semaphore;

    app()->render_queue()->vkQueueSubmit(
        app()->render_queue(), 1, &init_submit_info, ::VkFence(ready_fence));

    if (application_.HasSeparatePresentQueue()) {
      ::VkSemaphore transfer_semaphore =
          *frame_data_[image_idx].transfer_semaphore_;
      present_ready_semaphore = render_wait_semaphore;
      VkSubmitInfo transfer_submit_info{
          VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
          nullptr,                        // pNext
          1,                              // waitSemaphoreCount
          &transfer_semaphore,            // pWaitSemaphores
          &flags,                         // pWaitDstStageMask,
          1,                              // commandBufferCount
          &(frame_data_[image_idx]
                .transfer_from_graphics_command_buffer_->get_command_buffer()),
          1,                        // signalSemaphoreCount
          &present_ready_semaphore  // pSignalSemaphores
      };

      app()->present_queue()->vkQueueSubmit(
          app()->present_queue(), 1, &transfer_submit_info,
          static_cast<::VkFence>(VK_NULL_HANDLE));
    }

    VkPresentInfoKHR present_info{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,    // sType
        nullptr,                               // pNext
        1,                                     // waitSemaphoreCount
        &present_ready_semaphore,              // pWaitSemaphores
        1,                                     // swapchainCount
        &app()->swapchain().get_raw_object(),  // pSwapchains
        &image_idx,                            // pImageIndices
        nullptr,                               // pResults
    };
    LOG_ASSERT(==, app()->GetLogger(),
               app()->present_queue()->vkQueuePresentKHR(app()->present_queue(),
                                                         &present_info),
               VK_SUCCESS);
  }

  void set_invalid(bool invaid) { is_valid_ = false; }
  const bool is_valid() { return is_valid_; }

  bool should_exit() const { return app()->should_exit(); }

 private:
  const size_t sample_frame_data_offset =
      reinterpret_cast<size_t>(
          &(reinterpret_cast<SampleFrameData*>(4096)->child_data_)) -
      4096;

 public:
  const ::VkImageView& depth_view(FrameData* data) {
    SampleFrameData* base = reinterpret_cast<SampleFrameData*>(
        reinterpret_cast<uint8_t*>(data) - sample_frame_data_offset);
    return base->depth_view_->get_raw_object();
  }

  const ::VkImageView& color_view(FrameData* data) {
    SampleFrameData* base = reinterpret_cast<SampleFrameData*>(
        reinterpret_cast<uint8_t*>(data) - sample_frame_data_offset);
    return base->image_view->get_raw_object();
  }

  const ::VkImage& swapchain_image(FrameData* data) {
    SampleFrameData* base = reinterpret_cast<SampleFrameData*>(
        reinterpret_cast<uint8_t*>(data) - sample_frame_data_offset);
    return base->swapchain_image_;
  }

  const ::VkImage& depth_image(FrameData* data) {
    SampleFrameData* base = reinterpret_cast<SampleFrameData*>(
        reinterpret_cast<uint8_t*>(data) - sample_frame_data_offset);
    return base->depth_stencil_->get_raw_image();
  }

 private:
  // This will be called during Initialize(). The application is expected
  // to initialize any frame-specific data that it needs.
  virtual void InitializeFrameData(
      FrameData* data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) = 0;
  // This will be called during Initialize(). The application is expected
  // to intialize any non frame-specific data here, such as images and
  // buffers.
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) = 0;

  // This will be called during Initialize(). The application is expected
  // to intialize any non frame-specific data here, such as images and
  // buffers.
  virtual void InitializationComplete() {}

  // Will be called to instruct the application to update it's non
  // frame-specific data.
  virtual void Update(float time_since_last_render) = 0;

  // Will be called to instruct the application to enqueue the necessary
  // commands for rendering frame <frame_index> into the provided queue
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      FrameData* data) = 0;

  // This initializes the per-frame data for the sample application framework.
  //  This is equivalent to the InitializeFrameData(), except this handles
  //  all of the under-the-hood data that the application itself should not
  //  have to worry about.
  void InitializeLocalFrameData(SampleFrameData* data,
                                vulkan::VkCommandBuffer* initialization_buffer,
                                size_t frame_index) {
    data->swapchain_image_ = swapchain_images_[frame_index];

    data->ready_semaphore_ = containers::make_unique<vulkan::VkSemaphore>(
        allocator_, vulkan::CreateSemaphore(&application_.device()));

    data->ready_fence_ = containers::make_unique<vulkan::VkFence>(
        allocator_, vulkan::CreateFence(&application_.device()));

    VkImageCreateInfo image_create_info{
        /* sType = */
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ kDepthFormat,
        /* extent = */
        {
            /* width = */ application_.swapchain().width(),
            /* height = */ application_.swapchain().height(),
            /* depth = */ application_.swapchain().depth(),
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ num_samples_,
        /* tiling = */
        VK_IMAGE_TILING_OPTIMAL,
        /* usage = */
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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
        kDepthFormat,                              // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_view;

    if (options_.enable_depth_buffer) {
      data->depth_stencil_ =
          application_.CreateAndBindImage(&image_create_info);
      view_create_info.image = *data->depth_stencil_;

      LOG_ASSERT(
          ==, data_->logger(), VK_SUCCESS,
          application_.device()->vkCreateImageView(
              application_.device(), &view_create_info, nullptr, &raw_view));
      data->depth_view_ = containers::make_unique<vulkan::VkImageView>(
          allocator_,
          vulkan::VkImageView(raw_view, nullptr, &application_.device()));
    }

    if (options_.enable_multisampling) {
      image_create_info.format = render_target_format_;
      image_create_info.usage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

      data->multisampled_target_ =
          application_.CreateAndBindImage(&image_create_info);
    }

    view_create_info.image = options_.enable_multisampling
                                 ? *data->multisampled_target_
                                 : data->swapchain_image_;
    view_create_info.format = render_target_format_;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    LOG_ASSERT(
        ==, data_->logger(), VK_SUCCESS,
        application_.device()->vkCreateImageView(
            application_.device(), &view_create_info, nullptr, &raw_view));
    data->image_view = containers::make_unique<vulkan::VkImageView>(
        allocator_,
        vulkan::VkImageView(raw_view, nullptr, &application_.device()));

    VkImageMemoryBarrier barriers[2] = {
        {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,            // sType
         nullptr,                                           // pNext
         0,                                                 // srcAccessMask
         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,      // dstAccessMask
         VK_IMAGE_LAYOUT_UNDEFINED,                         // oldLayout
         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // newLayout
         VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
         VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
         options_.enable_depth_buffer
             ? static_cast<::VkImage>(*data->depth_stencil_)
             : static_cast<::VkImage>(VK_NULL_HANDLE),  // image
         {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}},
        {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
         nullptr,                                   // pNext
         0,                                         // srcAccessMask
         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
         VK_IMAGE_LAYOUT_UNDEFINED,                 // oldLayout
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
         VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
         VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
         options_.enable_multisampling ? *data->multisampled_target_
                                       : data->swapchain_image_,  // image
         {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}}};

    (*initialization_buffer)
        ->vkCmdPipelineBarrier(
            (*initialization_buffer), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr,
            options_.enable_depth_buffer ? 2 : 1,
            &barriers[options_.enable_depth_buffer ? 0 : 1]);

    uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    if (application_.HasSeparatePresentQueue()) {
      data->transfer_semaphore_ = containers::make_unique<vulkan::VkSemaphore>(
          allocator_, vulkan::CreateSemaphore(&application_.device()));
      srcQueueFamilyIndex = application_.present_queue().index();
      dstQueueFamilyIndex = application_.render_queue().index();
      VkImageMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
          nullptr,                                 // pNext
          0,                                       // srcAccessMask
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // dstAccessMask
          VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
          VK_IMAGE_LAYOUT_UNDEFINED,               // newLayout
          srcQueueFamilyIndex,                     // srcQueueFamilyIndex
          dstQueueFamilyIndex,                     // dstQueueFamilyIndex
          data->swapchain_image_,                  // image
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

      data->transfer_from_present_command_buffer_ =
          containers::make_unique<vulkan::VkCommandBuffer>(
              allocator_, app()->GetCommandBuffer());

      (*data->transfer_from_present_command_buffer_)
          ->vkBeginCommandBuffer((*data->transfer_from_present_command_buffer_),
                                 &kBeginCommandBuffer);

      (*data->transfer_from_present_command_buffer_)
          ->vkCmdPipelineBarrier((*data->transfer_from_present_command_buffer_),
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);
      (*data->transfer_from_present_command_buffer_)
          ->vkEndCommandBuffer(*data->transfer_from_present_command_buffer_);

      barrier.srcQueueFamilyIndex = dstQueueFamilyIndex;
      barrier.dstQueueFamilyIndex = srcQueueFamilyIndex;
      barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      data->transfer_from_graphics_command_buffer_ =
          containers::make_unique<vulkan::VkCommandBuffer>(
              allocator_, app()->GetCommandBuffer());

      (*data->transfer_from_graphics_command_buffer_)
          ->vkBeginCommandBuffer((*data->setup_command_buffer_),
                                 &kBeginCommandBuffer);

      (*data->transfer_from_graphics_command_buffer_)
          ->vkCmdPipelineBarrier((*data->setup_command_buffer_),
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);
      (*data->transfer_from_graphics_command_buffer_)
          ->vkEndCommandBuffer(*data->setup_command_buffer_);
    }

    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        0,                                         // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                 // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
        srcQueueFamilyIndex,                       // srcQueueFamilyIndex
        dstQueueFamilyIndex,                       // dstQueueFamilyIndex
        options_.enable_multisampling ? *data->multisampled_target_
                                      : data->swapchain_image_,  // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    data->setup_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            allocator_, app()->GetCommandBuffer());

    (*data->setup_command_buffer_)
        ->vkBeginCommandBuffer((*data->setup_command_buffer_),
                               &kBeginCommandBuffer);

    (*data->setup_command_buffer_)
        ->vkCmdPipelineBarrier((*data->setup_command_buffer_),
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                               0, nullptr, 0, nullptr, 1, &barrier);
    (*data->setup_command_buffer_)
        ->vkEndCommandBuffer(*data->setup_command_buffer_);

    data->resolve_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            allocator_, app()->GetCommandBuffer());

    (*data->resolve_command_buffer_)
        ->vkBeginCommandBuffer((*data->resolve_command_buffer_),
                               &kBeginCommandBuffer);
    VkImageLayout old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAccessFlags old_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (options_.enable_multisampling) {
      old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      old_access = VK_ACCESS_TRANSFER_WRITE_BIT;
      VkImageMemoryBarrier resolve_barrier[2] = {
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
           nullptr,                                   // pNext
           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // srcAccessMask
           VK_ACCESS_TRANSFER_READ_BIT,               // dstAccessMask
           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // oldLayout
           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,      // newLayout
           VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
           VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
           *data->multisampled_target_,               // image
           {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
           nullptr,                                 // pNext
           0,                                       // srcAccessMask
           VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
           VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
           VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
           VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
           data->swapchain_image_,                  // image
           {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}}};
      (*data->resolve_command_buffer_)
          ->vkCmdPipelineBarrier(*data->resolve_command_buffer_,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                 0, nullptr, 2, resolve_barrier);
      VkImageResolve region = {
          {
              VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
              0,                          // mipLevel
              0,                          // baseArrayLayer
              1,                          // layerCount
          },                              // srcSubresource
          {
              0,  // x
              0,  // y
              0   // z
          },      // srcOffset
          {
              VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
              0,                          // mipLevel
              0,                          // baseArrayLayer
              1,                          // layerCount
          },                              // dstSubresource
          {
              0,  // x
              0,  // y
              0   // z
          },      // dstOffset
          {
              app()->swapchain().width(),   // width
              app()->swapchain().height(),  // height
              1                             // depth
          }                                 // extent
      };
      (*data->resolve_command_buffer_)
          ->vkCmdResolveImage(
              (*data->resolve_command_buffer_), *data->multisampled_target_,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, data->swapchain_image_,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    VkImageMemoryBarrier present_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        old_access,                              // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,               // dstAccessMask
        old_layout,                              // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,         // newLayout
        dstQueueFamilyIndex,                     // srcQueueFamilyIndex
        srcQueueFamilyIndex,                     // dstQueueFamilyIndex
        data->swapchain_image_,                  // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    (*data->resolve_command_buffer_)
        ->vkCmdPipelineBarrier((*data->resolve_command_buffer_),
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                               nullptr, 0, nullptr, 1, &present_barrier);
    (*data->resolve_command_buffer_)
        ->vkEndCommandBuffer(*data->resolve_command_buffer_);

    InitializeFrameData(&data->child_data_, initialization_buffer, frame_index);
  }

  SampleOptions options_;
  const entry::EntryData* data_;
  containers::Allocator* allocator_;
  // The VulkanApplication that we build on, we want this to be the
  // last thing deleted, it goes at the top.
  vulkan::VulkanApplication application_;

  // This contains one SampleFrameData per swapchain image. It will be used
  // to render frames to the appropriate swapchains
  containers::vector<SampleFrameData> frame_data_;
  // The number of samples that we will render with
  VkSampleCountFlagBits num_samples_;
  // The format of our render_target
  VkFormat render_target_format_;
  // The viewport we use to render
  VkViewport default_viewport_;
  // The scissor we use to render
  VkRect2D default_scissor_;
  // The last time ProcessFrame was called. This is used to calculate the
  // delta
  //  to be passed to Update.
  std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time_;
  // Do not move these above application_, they rely on the fact that
  // application_ will be initialized first.
  const containers::vector<::VkImage>& swapchain_images_;
  // The command buffer used to intialize all of the data.
  vulkan::VkCommandBuffer initialization_command_buffer_;
  // The exponentially smoothed average frame time.
  float average_frame_time_;
  // If this is set to false, the application cannot be safely run.
  bool is_valid_;
};  // namespace sample_application
}  // namespace sample_application

#endif  // SAMPLE_APPLICATION_FRAMEWORK_SAMPLE_APPLICATION_H_
