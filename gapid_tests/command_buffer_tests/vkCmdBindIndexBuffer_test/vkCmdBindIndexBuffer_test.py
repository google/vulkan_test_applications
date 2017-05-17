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


@gapit_test("vkCmdBindIndexBuffer_test")
class U32Buffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        cmd_bind_vertex_buffers = require(
            self.nth_call_of("vkCmdBindIndexBuffer", 1))

        require_not_equal(0, cmd_bind_vertex_buffers.int_commandBuffer)
        require_not_equal(0, cmd_bind_vertex_buffers.int_buffer)
        require_equal(0, cmd_bind_vertex_buffers.int_offset)
        require_equal(VK_INDEX_TYPE_UINT32,
                      cmd_bind_vertex_buffers.int_indexType)


@gapit_test("vkCmdBindIndexBuffer_test")
class U16Buffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        cmd_bind_vertex_buffers = require(
            self.nth_call_of("vkCmdBindIndexBuffer", 2))

        require_not_equal(0, cmd_bind_vertex_buffers.int_commandBuffer)
        require_not_equal(0, cmd_bind_vertex_buffers.int_buffer)
        require_equal(128 * 2, cmd_bind_vertex_buffers.int_offset)
        require_equal(VK_INDEX_TYPE_UINT16,
                      cmd_bind_vertex_buffers.int_indexType)
