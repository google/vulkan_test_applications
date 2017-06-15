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
from gapit_test_framework import GapitTest, get_read_offset_function, get_write_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY, DEVICE_SIZE, INT32_T
from vulkan_constants import *

COMMAND_BUFFER_ALLOCATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("commandPool", HANDLE),
    ("level", UINT32_T),
    ("commandBufferCount", UINT32_T),
]

MAX_COUNT = 5


@gapit_test("AllocateResetFreeCommandBuffer")
class AllocateAndFreeCommandBuffers(GapitTest):

    def expect(self):
        """Check the arguments to vkAllocateCommandBuffers"""
        architecture = self.architecture
        for level in [VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                      VK_COMMAND_BUFFER_LEVEL_SECONDARY]:
            for count in [1, 2, MAX_COUNT]:
                allocate = require(self.next_call_of(
                    "vkAllocateCommandBuffers"))
                require_not_equal(0, allocate.int_device)
                require_not_equal(0, allocate.hex_pAllocateInfo)
                require_not_equal(0, allocate.hex_pCommandBuffers)
                require_equal(VK_SUCCESS, int(allocate.return_val))

                allocate_info = VulkanStruct(
                    architecture, COMMAND_BUFFER_ALLOCATE_INFO,
                    get_read_offset_function(allocate,
                                             allocate.hex_pAllocateInfo))
                require_equal(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                              allocate_info.sType)
                command_pool = allocate_info.commandPool
                require_not_equal(0, command_pool)
                require_equal(level, allocate_info.level)
                require_equal(count, allocate_info.commandBufferCount)

                allocated_buffers = []
                if count > 0:
                    command_buffers = VulkanStruct(
                        architecture, [("handles", ARRAY, count, POINTER)],
                        get_write_offset_function(allocate,
                                                  allocate.hex_pCommandBuffers))
                    for i in range(count):
                        require_not_equal(0, command_buffers.handles[i])
                        allocated_buffers.append(command_buffers.handles[i])

                free = require(self.next_call_of("vkFreeCommandBuffers"))
                require_not_equal(0, free.int_device)
                require_equal(command_pool, free.int_commandPool)
                require_equal(count, free.int_commandBufferCount)
                freed_buffers = []
                if count > 0:
                    command_buffers = VulkanStruct(
                        architecture, [("handles", ARRAY, count, POINTER)],
                        get_read_offset_function(free,
                                                 free.hex_pCommandBuffers))
                    for i in range(count):
                        require_not_equal(0, command_buffers.handles[i])
                        freed_buffers.append(command_buffers.handles[i])

                require_equal(allocated_buffers, freed_buffers)


@gapit_test("AllocateResetFreeCommandBuffer")
class ResetCommandBuffer(GapitTest):

    def expect(self):
        """Check the arguments to vkResetCommandBuffer"""
        first_reset = require(self.next_call_of("vkResetCommandBuffer"))
        require_not_equal(0, first_reset.int_commandBuffer)
        require_equal(VK_SUCCESS, int(first_reset.return_val))

        second_reset = require(self.next_call_of("vkResetCommandBuffer"))
        require_not_equal(0, second_reset.int_commandBuffer)
        require_equal(VK_SUCCESS, int(second_reset.return_val))

        # As the |flags| is used in a circular way, the flags used for the first
        # and the second call to vkResetCommandBuffer must be different.
        require_not_equal(first_reset.int_flags, second_reset.int_flags)
