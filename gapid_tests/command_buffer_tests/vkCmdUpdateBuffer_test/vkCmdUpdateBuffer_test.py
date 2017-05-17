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


@gapit_test("vkCmdUpdateBuffer_test")
class MaximumBufferUpdateWithZeroOffset(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdUpdateBuffer"""
        update_buffer = require(
            self.nth_call_of("vkCmdUpdateBuffer", 1))

        require_not_equal(0, update_buffer.int_commandBuffer)
        require_not_equal(0, update_buffer.int_dstBuffer)
        require_equal(0, update_buffer.int_dstOffset)
        require_equal(65536, update_buffer.int_dataSize)
        require_not_equal(0, update_buffer.hex_pData)
