/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VULKAN_HELPERS_BUFFER_FRAME_DATA_H
#define VULKAN_HELPERS_BUFFER_FRAME_DATA_H

#include "vulkan_helpers/vulkan_application.h"

namespace vulkan {

const size_t kMaxOffsetAlignment = 256;

// Rounds to the given power_of_2
size_t RoundUp(size_t to_round, size_t power_of_2_to_round) {
  return (to_round + power_of_2_to_round - 1) & ~(power_of_2_to_round - 1);
}

template <typename T>
class BufferFrameData {
  // BufferFrameData is a class that wraps some amount of data for multi-frame
  // updates.
  // It handles the creation of uniform buffers, and can update them
  // when it is appropriate.
  // T can be any type that can be bitwise copied. The data will be mapped
  // byte for byte into a uniform buffer, so it it must have the proper
  // alignment as defined in SPIR-V.
 public:
  // |buffered_data_count| is the number of buffered frames the uniform data
  // should produce. Typcially this is one per swapchain image. |usage| is the
  // VkBufferUsageFlags used for the underlying VkBuffer(s) that stores the
  // uniform data. Note that VK_BUFFER_USAGE_TRANSFER_DST_BIT will be added
  // along with |usage| to guarantee data can be copied to the underlying
  // VkBuffer(s).
  BufferFrameData(VulkanApplication* application, size_t buffered_data_count,
                  VkBufferUsageFlags usage, uint32_t device_mask = 0, uint32_t queue_family_index = 0)
      : application_(application),
        uninitialized_(application->GetAllocator()),
        update_commands_(application->GetAllocator()),
        device_mask_(device_mask),
        queue_family_index_(queue_family_index) {
    uint32_t dm = device_mask;
    uninitialized_.insert(uninitialized_.begin(), buffered_data_count, true);
    const size_t aligned_data_size =
        RoundUp(sizeof(set_value_), kMaxOffsetAlignment);

    uint32_t indices[VK_MAX_DEVICE_GROUP_SIZE];

    uint32_t set = 0;
    for (uint32_t i = 0; dm != 0; ++i) {
      if (dm & 0x1) {
        // Host visible buffers can only exist on one GPU
        LOG_ASSERT(==, application_->GetLogger(), 0, set);
        set = i + 1;
      }
      dm >>= 1;
    }
    dm = device_mask;
    if (set != 0) {
      for (uint32_t i = 0; i < application_->device().num_devices(); ++i) {
        indices[i] = set - 1;
      }
    }

    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,      // sType
        nullptr,                                   // pNext
        0,                                         // flags
        aligned_data_size * buffered_data_count,   // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,  // usage
        VK_SHARING_MODE_EXCLUSIVE,
        1,
        &queue_family_index_};
    buffer_ = application_->CreateAndBindDeviceBuffer(
        &create_info, set == 0 ? nullptr : &indices[0]);

    create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    host_buffer_ = application_->CreateAndBindHostBuffer(
        &create_info, set == 0 ? nullptr : &indices[0]);

    VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr                                       // pInheritanceInfo
    };

    for (size_t i = 0; i < buffered_data_count; ++i) {
      update_commands_.push_back(application_->GetCommandBuffer(queue_family_index_));
      update_commands_.back()->vkBeginCommandBuffer(update_commands_.back(),
                                                    &begin_info);
      if (device_mask_ != 0) {
        update_commands_.back()->vkCmdSetDeviceMask(update_commands_.back(),
                                                      device_mask_); 
      }
      VkBufferMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
          nullptr,                                  // pNext
          VK_ACCESS_HOST_WRITE_BIT,                 // srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
          *host_buffer_,
          aligned_data_size * i,
          size()};

      update_commands_.back()->vkCmdPipelineBarrier(
          update_commands_.back(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0,
          nullptr);
      VkBufferCopy region{aligned_data_size * i, aligned_data_size * i, size()};

      update_commands_.back()->vkCmdCopyBuffer(
          update_commands_.back(), *host_buffer_, *buffer_, 1, &region);

      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
      update_commands_.back()->vkCmdPipelineBarrier(
          update_commands_.back(), VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
          nullptr);

      update_commands_.back()->vkEndCommandBuffer(update_commands_.back());
    }
  }

  T& data() { return set_value_; }

  // Enqueues an update operation on the queue if needed, to ensure
  // that the buffer is correct for the given index.
  void UpdateBuffer(VkQueue* update_queue, size_t buffer_index,
                    uint32_t kDeviceMask = 0, bool force = false) {
    const size_t offset = get_offset_for_frame(buffer_index);
    bool equal =
        memcmp(&set_value_, host_buffer_->base_address() + offset, size()) == 0;
    if (force || !equal || uninitialized_[buffer_index]) {
      // If the data for this frame is not what was previously recorded into
      // the buffer, then copy the data into the buffer and update it.
      uninitialized_[buffer_index] = false;
      memcpy(host_buffer_->base_address() + offset, &set_value_, size());
      host_buffer_->flush(offset, aligned_data_size());

      VkDeviceGroupSubmitInfo group_submit_info = {
          VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO,
          nullptr,
          0,
          nullptr,
          1,
          &device_mask_,
          0,
          nullptr};

      VkSubmitInfo init_submit_info{
          VK_STRUCTURE_TYPE_SUBMIT_INFO,                     // sType
          device_mask_ == 0 ? nullptr : &group_submit_info,  // pNext
          0,        // waitSemaphoreCount
          nullptr,  // pWaitSemaphores
          nullptr,  // pWaitDstStageMask,
          1,        // commandBufferCount
          &update_commands_[buffer_index].get_command_buffer(),
          0,       // signalSemaphoreCount
          nullptr  // pSignalSemaphores
      };
      (*update_queue)
          ->vkQueueSubmit(*update_queue, 1, &init_submit_info, ::VkFence(0));
    }
  }

  // Returns the Uniform buffer backing the uniform data.
  ::VkBuffer get_buffer() const { return *buffer_; }
  // Returns the offset in the buffer for each frame.
  size_t get_offset_for_frame(size_t buffer_index) const {
    return aligned_data_size() * buffer_index;
  }
  // Returns the size of the data used for each frame.
  size_t size() const { return sizeof(set_value_); }

  // Returns the aligned size of the data for each frame.
  size_t aligned_data_size() const {
    return RoundUp(size(), kMaxOffsetAlignment);
  }

 private:
  VulkanApplication* application_;
  containers::vector<bool> uninitialized_;
  // This is the actual host piece of data that can be updated by the user.
  T set_value_;
  // This is the gpu-side buffer that contains the uniforms.
  containers::unique_ptr<VulkanApplication::Buffer> buffer_;
  // This is the host-side buffer that contains the data that can be copied to
  // the uniforms.
  containers::unique_ptr<VulkanApplication::Buffer> host_buffer_;
  // These command-buffers contain the command needed to update the
  // device-buffer from the host buffer.
  containers::vector<VkCommandBuffer> update_commands_;
  uint32_t device_mask_;
  uint32_t queue_family_index_;
};
}  // namespace vulkan

#endif  // VULKAN_HELPERS_BUFFER_FRAME_DATA_H
