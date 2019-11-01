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

#include "vulkan_helpers/buffer_frame_data.h"
#ifdef __linux__
#include <unistd.h>
#endif

template <typename T>
class VkBufferExported {
 public:
  VkBufferExported(vulkan::VkDevice& device, logging::Logger* log,
                   size_t num_images)
      : device_(device),
        log_(log),
        buffer_(VK_NULL_HANDLE, nullptr, &device),
        device_memory_(VK_NULL_HANDLE, nullptr, &device),
        aligned_data_size_(
            vulkan::RoundUp(sizeof(T), vulkan::kMaxOffsetAlignment)) {
    VkBuffer buffer;

    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        aligned_data_size_ * num_images,       // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};

    LOG_ASSERT(==, log, VK_SUCCESS,
               device->vkCreateBuffer(device, &create_info, nullptr, &buffer));

    buffer_.initialize(buffer);

    VkMemoryRequirements requirements;
    device->vkGetBufferMemoryRequirements(device, buffer, &requirements);

    uint32_t memory_index =
        vulkan::GetMemoryIndex(&device, log, requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkExportMemoryAllocateInfo export_allocate_info{
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, nullptr,
#ifdef _WIN32
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#elif __linux__
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#endif
    };

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        &export_allocate_info,                   // pNext
        aligned_data_size_ * num_images,         // allocationSize
        memory_index};

    VkDeviceMemory device_memory;

    LOG_ASSERT(==, log, VK_SUCCESS,
               device->vkAllocateMemory(device, &allocate_info, nullptr,
                                        &device_memory));
    device_memory_.initialize(device_memory);

    device_->vkBindBufferMemory(device_, buffer, device_memory, 0);
  }

  ::VkBuffer get_buffer() const { return buffer_; }

  size_t size() const { return sizeof(T); }

  size_t aligned_data_size() const { return aligned_data_size_; }

  size_t get_offset_for_frame(size_t buffer_index) const {
    return aligned_data_size_ * buffer_index;
  }

#ifdef _WIN32
  HANDLE getMemoryWin32Handle(void) {
    HANDLE handle;
    VkMemoryGetWin32HandleInfoKHR get_handle_info{
        VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,  // sType
        nullptr,                                             // pNext
        device_memory_, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT};
    LOG_ASSERT(
        ==, log_, VK_SUCCESS,
        device_->vkGetMemoryWin32HandleKHR(device_, &get_handle_info, &handle));
    return handle;
  }
#elif __linux__
  int getMemoryFd(void) {
    int file_descriptor;
    VkMemoryGetFdInfoKHR get_fd_info{
        VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, nullptr, device_memory_,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT};
    LOG_ASSERT(
        ==, log_, VK_SUCCESS,
        device_->vkGetMemoryFdKHR(device_, &get_fd_info, &file_descriptor));
    return file_descriptor;
  }
#endif

 private:
  vulkan::VkDevice& device_;
  logging::Logger* log_;
  vulkan::VkBuffer buffer_;
  vulkan::VkDeviceMemory device_memory_;
  const size_t aligned_data_size_;
};

template <typename T>
class VkBufferImported {
 public:
  VkBufferImported(vulkan::VkDevice& device, logging::Logger* log,
                   size_t num_images,
#ifdef _WIN32
                   HANDLE handle
#elif __linux__
                   int fd
#endif
                   )
      : device_(device),
        log_(log),
        buffer_(VK_NULL_HANDLE, nullptr, &device),
        device_memory_(VK_NULL_HANDLE, nullptr, &device),
        aligned_data_size_(
            vulkan::RoundUp(sizeof(T), vulkan::kMaxOffsetAlignment)) {
    VkBuffer buffer;

    VkExternalMemoryBufferCreateInfo external_create_info{
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO, nullptr,
#ifdef _WIN32
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#elif __linux__
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#endif

    };

    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        aligned_data_size_ * num_images,       // size
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};

    LOG_ASSERT(==, log, VK_SUCCESS,
               device->vkCreateBuffer(device, &create_info, nullptr, &buffer));

    buffer_.initialize(buffer);

    VkMemoryRequirements requirements;
    device->vkGetBufferMemoryRequirements(device, buffer, &requirements);

    uint32_t memory_index =
        vulkan::GetMemoryIndex(&device, log, requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

#ifdef _WIN32
    VkImportMemoryWin32HandleInfoKHR import_allocate_info{
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, nullptr,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, handle, nullptr};
#elif __linux__
    VkImportMemoryFdInfoKHR import_allocate_info{
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, nullptr,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, fd};
#endif

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        &import_allocate_info,                   // pNext
        aligned_data_size_ * num_images,         // allocationSize
        memory_index};

    VkDeviceMemory device_memory;

    LOG_ASSERT(==, log, VK_SUCCESS,
               device->vkAllocateMemory(device, &allocate_info, nullptr,
                                        &device_memory));
    device_memory_.initialize(device_memory);

    device_->vkBindBufferMemory(device_, buffer, device_memory, 0);
  }

  ~VkBufferImported() {}

  ::VkBuffer get_buffer() const { return buffer_; }

  size_t size() const { return sizeof(T); }

  size_t get_offset_for_frame(size_t buffer_index) const {
    return aligned_data_size_ * buffer_index;
  }

 private:
  vulkan::VkDevice& device_;
  logging::Logger* log_;
  vulkan::VkBuffer buffer_;
  vulkan::VkDeviceMemory device_memory_;
  const size_t aligned_data_size_;
};
