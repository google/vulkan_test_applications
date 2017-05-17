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
from gapit_test_framework import GapitTest, get_read_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY, DEVICE_SIZE
from vulkan_constants import *

@gapit_test("vkCmdBindVertexBuffers_test")
class SingleBuffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        cmd_bind_vertex_buffers = require(
            self.next_call_of("vkCmdBindVertexBuffers"))

        require_not_equal(0, cmd_bind_vertex_buffers.int_commandBuffer)
        require_equal(0, cmd_bind_vertex_buffers.int_firstBinding)
        require_equal(1, cmd_bind_vertex_buffers.int_bindingCount)
        require_not_equal(0, cmd_bind_vertex_buffers.hex_pBuffers)
        require_not_equal(0, cmd_bind_vertex_buffers.hex_pOffsets)

        sent_buffer = little_endian_bytes_to_int(
                require(
                    cmd_bind_vertex_buffers.get_read_data(
                        cmd_bind_vertex_buffers.hex_pBuffers,
                        NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(sent_buffer, 0)

        sent_offset = little_endian_bytes_to_int(
            require(
                cmd_bind_vertex_buffers.get_read_data(
                    cmd_bind_vertex_buffers.hex_pOffsets,
                    8)))
        require_equal(0, sent_offset)

BUFFER_COPY = [
    ("srcOffset", DEVICE_SIZE),
    ("dstOffset", DEVICE_SIZE),
    ("size", DEVICE_SIZE)
]

@gapit_test("vkCmdBindVertexBuffers_test")
class CopyBuffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        cmd_copy_buffer = require(
            self.next_call_of("vkCmdCopyBuffer"))

        require_not_equal(0, cmd_copy_buffer.int_commandBuffer)
        require_not_equal(0, cmd_copy_buffer.int_srcBuffer)
        require_not_equal(0, cmd_copy_buffer.int_dstBuffer)
        require_equal(1, cmd_copy_buffer.int_regionCount)
        require_not_equal(0, cmd_copy_buffer.hex_pRegions)

        copy = VulkanStruct(
            architecture, BUFFER_COPY,
            get_read_offset_function(cmd_copy_buffer,
                cmd_copy_buffer.hex_pRegions))
        require_equal(0, copy.srcOffset)
        require_equal(0, copy.dstOffset)
        require_equal(1024, copy.size)
