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
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY, DEVICE_SIZE, INT32_T
from vulkan_constants import *


@gapit_test("vkCmdFillBuffer_test")
class FillBufferWithWholeSizeAndZeroOffset(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdFillBuffer"""
        fill_buffer = require(
            self.nth_call_of("vkCmdFillBuffer", 1))

        require_not_equal(0, fill_buffer.int_commandBuffer)
        require_not_equal(0, fill_buffer.int_dstBuffer)
        require_equal(0, fill_buffer.int_dstOffset)
        require_equal(VK_WHOLE_SIZE, fill_buffer.int_size)
        require_equal(0x12345678, fill_buffer.int_data)


@gapit_test("vkCmdFillBuffer_test")
class FillBufferWithNonWholeSizeAndNonZeroOffset(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdFillBuffer"""
        fill_buffer = require(
            self.nth_call_of("vkCmdFillBuffer", 2))

        require_not_equal(0, fill_buffer.int_commandBuffer)
        require_not_equal(0, fill_buffer.int_dstBuffer)
        require_equal(128, fill_buffer.int_dstOffset)
        require_equal(256, fill_buffer.int_size)
        require_equal(0xabcdabcd, fill_buffer.int_data)
