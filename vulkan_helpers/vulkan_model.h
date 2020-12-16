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

#ifndef VULKAN_HELPERS_VULKAN_MODEL_H_
#define VULKAN_HELPERS_VULKAN_MODEL_H_

#include <initializer_list>

#include "support/containers/allocator.h"
#include "support/containers/vector.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_application.h"

namespace vulkan {
const size_t POSITION_SIZE = sizeof(float) * 3;
const size_t TEXCOORD_SIZE = sizeof(float) * 2;
const size_t NORMAL_SIZE = sizeof(float) * 3;

const size_t INDEX_SIZE = sizeof(uint32_t);

struct VulkanModel {
 public:
  // A standard VulkanModel object. It is expected to be used with the
  // output from convert_obj_to_c.py. That is, positions, texture_coords
  // and normals are expected to be sequential in memory.
  VulkanModel(containers::Allocator* allocator, logging::Logger* logger,
              size_t num_vertices, const float* positions,
              const float* texture_coords, const float* normals,
              size_t num_indices, const uint32_t* indices)
      : allocator_(allocator),
        logger_(logger),
        positions_(positions),
        texture_coords_(texture_coords),
        normals_(normals),
        indices_(indices),
        num_vertices_(num_vertices),
        num_indices_(num_indices),
        vertex_data_size_(num_vertices *
                          (POSITION_SIZE + TEXCOORD_SIZE + NORMAL_SIZE)),
        index_data_size_(num_indices * INDEX_SIZE) {
    // Make sure that vertices, indices and normals are contiguous in memory
    // this simplifies everything. The standard model format guarantees this.
    LOG_ASSERT(==, logger, texture_coords,
               reinterpret_cast<const float*>(
                   reinterpret_cast<const uint8_t*>(positions) +
                   num_vertices * POSITION_SIZE));
    LOG_ASSERT(==, logger, normals,
               reinterpret_cast<const float*>(
                   reinterpret_cast<const uint8_t*>(texture_coords) +
                   num_vertices * TEXCOORD_SIZE));
  }

  // Constructs a vulkan model from the output of the convert_obj_to_c.py
  // script.
  template <typename T>
  VulkanModel(containers::Allocator* allocator, logging::Logger* logger,
              const T& t)
      : VulkanModel(allocator, logger, t.num_vertices, t.positions, t.uv,
                    t.normals, t.num_indices, t.indices) {}

  // Creates the vertex and index buffers. Adds transfer commands to
  // CmdBuffer to populate the vertex and index buffers with data.
  // If this model has already been initialized, then this re-initializes it.
  // TODO(awoloszyn):
  // Currently all uploads are done with vkCmdUpdateBuffer. If many large models
  // are created, then switch to a staging + data buffer.
  void InitializeData(vulkan::VulkanApplication* application,
                      vulkan::VkCommandBuffer* cmdBuffer) {
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        vertex_data_size_,                     // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};
    vertexBuffer_ = application->CreateAndBindDeviceBuffer(&create_info);
    application->FillSmallBuffer(
        vertexBuffer_.get(), static_cast<const void*>(positions_),
        vertex_data_size_, 0, cmdBuffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

    create_info.usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    create_info.size = index_data_size_;

    indexBuffer_ = application->CreateAndBindDeviceBuffer(&create_info);

    application->FillSmallBuffer(
        indexBuffer_.get(), static_cast<const void*>(indices_),
        index_data_size_, 0, cmdBuffer, VK_ACCESS_INDEX_READ_BIT);
  }

  // Releases all resources held by this model.
  void ReleaseData() {
    vertexBuffer_.release();
    indexBuffer_.release();
  }

  struct InputStateAssemblyInfo {};

  // Adds the vertex assembly state to the given vectors
  // for this model to be used in a pipeline.
  // The attributes for the vertices are bound to sequential binding numbers:
  //    layout(location = 0) in vec3 positions_;
  //    layout(location = 1) in vec2 texture_coords_;
  //    layout(location = 2) in vec3 normals_;
  void GetAssemblyInfo(
      containers::vector<VkVertexInputBindingDescription>* input_bindings,
      containers::vector<VkVertexInputAttributeDescription>*
          vertex_attribute_descriptions) {
    input_bindings->push_back({
        0,                           // binding
        POSITION_SIZE,               // stride
        VK_VERTEX_INPUT_RATE_VERTEX  // inputRate
    });
    input_bindings->push_back({
        1,                           // binding
        TEXCOORD_SIZE,               // stride
        VK_VERTEX_INPUT_RATE_VERTEX  // inputRate
    });
    input_bindings->push_back({
        2,                           // binding
        NORMAL_SIZE,                 // stride
        VK_VERTEX_INPUT_RATE_VERTEX  // inputRate
    });

    vertex_attribute_descriptions->push_back({
        0,                           // location
        0,                           // binding
        VK_FORMAT_R32G32B32_SFLOAT,  // format
        0,                           // offset
    });
    vertex_attribute_descriptions->push_back({
        1,                        // location
        1,                        // binding
        VK_FORMAT_R32G32_SFLOAT,  // format
        0,                        // offset
    });
    vertex_attribute_descriptions->push_back({
        2,                           // location
        2,                           // binding
        VK_FORMAT_R32G32B32_SFLOAT,  // format
        0,                           // offset
    });
  }

  // Inserts the commands required to draw this model into the given
  // command-buffer. This binds the vertex and index buffers, and issues
  // the draw call.
  void Draw(vulkan::VkCommandBuffer* cmdBuffer) {
    ::VkBuffer buffers[3] = {*vertexBuffer_, *vertexBuffer_, *vertexBuffer_};
    ::VkDeviceSize offsets[3] = {
        0, num_vertices_ * POSITION_SIZE,
        num_vertices_ * (POSITION_SIZE + TEXCOORD_SIZE)};
    (*cmdBuffer)->vkCmdBindVertexBuffers(*cmdBuffer, 0, 3, buffers, offsets);
    (*cmdBuffer)
        ->vkCmdBindIndexBuffer(*cmdBuffer, *indexBuffer_, 0,
                               VK_INDEX_TYPE_UINT32);
    (*cmdBuffer)
        ->vkCmdDrawIndexed(*cmdBuffer, static_cast<uint32_t>(num_indices_), 1,
                           0, 0, 0);
  }

  // Draws an instanced version of this model.
  void DrawInstanced(vulkan::VkCommandBuffer* cmdBuffer,
                     uint32_t instance_count) {
    ::VkBuffer buffers[3] = {*vertexBuffer_, *vertexBuffer_, *vertexBuffer_};
    ::VkDeviceSize offsets[3] = {
        0, num_vertices_ * POSITION_SIZE,
        num_vertices_ * (POSITION_SIZE + TEXCOORD_SIZE)};
    (*cmdBuffer)->vkCmdBindVertexBuffers(*cmdBuffer, 0, 3, buffers, offsets);
    (*cmdBuffer)
        ->vkCmdBindIndexBuffer(*cmdBuffer, *indexBuffer_, 0,
                               VK_INDEX_TYPE_UINT32);
    (*cmdBuffer)
        ->vkCmdDrawIndexed(*cmdBuffer, static_cast<uint32_t>(num_indices_),
                           instance_count, 0, 0, 0);
  }

  void BindVertexAndIndexBuffers(vulkan::VkCommandBuffer* cmdBuffer) {
    ::VkBuffer buffers[3] = {*vertexBuffer_, *vertexBuffer_, *vertexBuffer_};
    ::VkDeviceSize offsets[3] = {
        0, num_vertices_ * POSITION_SIZE,
        num_vertices_ * (POSITION_SIZE + TEXCOORD_SIZE)};
    (*cmdBuffer)->vkCmdBindVertexBuffers(*cmdBuffer, 0, 3, buffers, offsets);
    (*cmdBuffer)
        ->vkCmdBindIndexBuffer(*cmdBuffer, *indexBuffer_, 0,
                               VK_INDEX_TYPE_UINT32);
  }

  size_t NumIndices() const { return num_indices_; }

 private:
  const float* positions_;
  const float* texture_coords_;
  const float* normals_;
  const uint32_t* indices_;
  size_t num_vertices_;
  size_t num_indices_;
  containers::Allocator* allocator_;
  logging::Logger* logger_;
  const size_t vertex_data_size_;
  const size_t index_data_size_;

  containers::unique_ptr<vulkan::VulkanApplication::Buffer> vertexBuffer_;
  containers::unique_ptr<vulkan::VulkanApplication::Buffer> indexBuffer_;
};

}  // namespace vulkan
#endif  // VULKAN_HELPERS_VULKAN_MODEL_H_