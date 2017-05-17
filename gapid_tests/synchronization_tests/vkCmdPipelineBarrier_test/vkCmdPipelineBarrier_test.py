# Copyright 2017 Google Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest
from struct_offsets import *
from vulkan_constants import *

IMAGE_MEMORY_BARRIER = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("srcAccessMask", UINT32_T),
    ("dstAccessMask", UINT32_T),
    ("oldLayout", UINT32_T),
    ("newLayout", UINT32_T),
    ("srcQueueFamilyIndex", UINT32_T),
    ("dstQueueFamilyIndex", UINT32_T),
    ("image", HANDLE),
    # These nested struct offsets are correct because all of the alignments
    # are trivially satisfied.
    ("subresourceRange_aspectMask", UINT32_T),
    ("subresourceRange_baseMipLevel", UINT32_T),
    ("subresourceRange_mipCount", UINT32_T),
    ("subresourceRange_baseArrayLayer", UINT32_T),
    ("subresourceRange_layerCount", UINT32_T)
]


@gapit_test("vkCmdPipelineBarrier_test")
class EmptyPipelineBarrierTest(GapitTest):

    def expect(self):
        """Expect that the applicationInfoPointer is null for the first
         vkCreateInstance"""
        pipeline_barrier = require(self.nth_call_of("vkCmdPipelineBarrier", 1))
        require_not_equal(pipeline_barrier.int_commandBuffer, 0)
        require_equal(pipeline_barrier.int_srcStageMask,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
        require_equal(pipeline_barrier.int_dstStageMask,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        require_equal(pipeline_barrier.int_dependencyFlags,
                      VK_DEPENDENCY_BY_REGION_BIT)
        require_equal(pipeline_barrier.int_memoryBarrierCount, 0)
        require_equal(pipeline_barrier.hex_pMemoryBarriers, 0)
        require_equal(pipeline_barrier.int_bufferMemoryBarrierCount, 0)
        require_equal(pipeline_barrier.hex_pBufferMemoryBarriers, 0)
        require_equal(pipeline_barrier.int_imageMemoryBarrierCount, 0)
        require_equal(pipeline_barrier.hex_pImageMemoryBarriers, 0)


@gapit_test("vkCmdPipelineBarrier_test")
class DefaultSwapchainMemoryBarrierTest(GapitTest):

    def expect(self):
        """Expect that the applicationInfoPointer is null for the first
         vkCreateInstance"""
        architecture = self.architecture
        pipeline_barrier = require(self.nth_call_of("vkCmdPipelineBarrier", 2))
        require_not_equal(pipeline_barrier.int_commandBuffer, 0)
        require_equal(pipeline_barrier.int_srcStageMask,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                      | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)
        require_equal(pipeline_barrier.int_dstStageMask,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                      | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
        require_equal(pipeline_barrier.int_dependencyFlags, 0)
        require_equal(pipeline_barrier.int_memoryBarrierCount, 0)
        require_equal(pipeline_barrier.hex_pMemoryBarriers, 0)
        require_equal(pipeline_barrier.int_bufferMemoryBarrierCount, 0)
        require_equal(pipeline_barrier.hex_pBufferMemoryBarriers, 0)
        require_equal(pipeline_barrier.int_imageMemoryBarrierCount, 1)
        require_not_equal(pipeline_barrier.hex_pImageMemoryBarriers, 0)

        def get_image_barrier_data(offset, size):
            return little_endian_bytes_to_int(
                require(
                    pipeline_barrier.get_read_data(
                        pipeline_barrier.hex_pImageMemoryBarriers + offset,
                        size)))

        image_memory_barrier = VulkanStruct(architecture, IMAGE_MEMORY_BARRIER,
                                            get_image_barrier_data)
        require_equal(image_memory_barrier.sType,
                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)
        require_equal(image_memory_barrier.pNext, 0)
        require_equal(image_memory_barrier.srcAccessMask,
                      VK_ACCESS_MEMORY_READ_BIT)
        require_equal(image_memory_barrier.dstAccessMask,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        require_equal(image_memory_barrier.oldLayout,
                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        require_equal(image_memory_barrier.newLayout,
                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        require_equal(image_memory_barrier.subresourceRange_aspectMask,
                      VK_IMAGE_ASPECT_COLOR_BIT)
        require_equal(image_memory_barrier.subresourceRange_baseMipLevel, 0)
        require_equal(image_memory_barrier.subresourceRange_mipCount, 1)
        require_equal(image_memory_barrier.subresourceRange_baseArrayLayer, 0)
        require_equal(image_memory_barrier.subresourceRange_layerCount, 1)
