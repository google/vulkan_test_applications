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

#include <algorithm>

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  vulkan::VkDevice& device = app.device();
  {
    // Creates one query pool with 4 queries, another one with 7 queries, then
    // resets the first pool with firstQuery: 0, queryCount: 4, resets the
    // second pool with firstQuery: 1, queryCount: 5
    vulkan::VkQueryPool four_queries_pool = vulkan::CreateQueryPool(
        &device,
        {
            VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            VK_QUERY_TYPE_OCCLUSION,                   // queryType
            4,                                         // queryCount
            0,                                         // pipelineStatistics
        });
    vulkan::VkQueryPool seven_queries_pool = vulkan::CreateQueryPool(
        &device,
        {
            VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            VK_QUERY_TYPE_OCCLUSION,                   // queryType
            7,                                         // queryCount
            0,                                         // pipelineStatistics
        });

    // Get command buffer and call the vkCmdResetQueryPool command
    vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    VkCommandBufferBeginInfo cmd_buf_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);
    cmd_buf->vkCmdResetQueryPool(cmd_buf,            // commandBuffer
                                 four_queries_pool,  // queryPool
                                 0,                  // firstQuery
                                 4                   // queryCount
    );
    cmd_buf->vkCmdResetQueryPool(cmd_buf,             // commandBuffer
                                 seven_queries_pool,  // queryPool
                                 1,                   // firstQuery
                                 5                    // queryCount
    );
    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             nullptr,
                             0,
                             nullptr,
                             nullptr,
                             1,
                             &raw_cmd_buf,
                             0,
                             nullptr};
    app.render_queue()->vkQueueSubmit(app.render_queue(), 1, &submit_info,
                                      static_cast<VkFence>(VK_NULL_HANDLE));
    app.render_queue()->vkQueueWaitIdle(app.render_queue());
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
