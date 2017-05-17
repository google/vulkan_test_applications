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
from gapit_test_framework import get_write_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, DEVICE_SIZE, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *


@gapit_test("BindMapUnmapBuffer_test")
class BindUnbindWholeMemoryRange(GapitTest):

    def expect(self):
        architecture = self.architecture
        bind_buffer_memory = require(self.next_call_of("vkBindBufferMemory"))
        map_memory = require(self.next_call_of("vkMapMemory"))
        unmap_memory = require(self.next_call_of("vkUnmapMemory"))

        require_not_equal(0, bind_buffer_memory.int_device)
        require_not_equal(0, bind_buffer_memory.int_buffer)
        require_not_equal(0, bind_buffer_memory.int_memory)
        require_equal(0, bind_buffer_memory.int_memoryOffset)

        require_equal(bind_buffer_memory.int_device, map_memory.int_device)
        require_equal(bind_buffer_memory.int_memory, map_memory.int_memory)
        require_equal(0, map_memory.int_offset)
        require_equal(VK_WHOLE_SIZE, map_memory.int_size)
        require_equal(0, map_memory.int_flags)
        require_not_equal(0, map_memory.hex_ppData)

        written_pointer = little_endian_bytes_to_int(
            require(
                map_memory.get_write_data(map_memory.hex_ppData,
                                          architecture.int_pointerSize)))

        require_not_equal(0, written_pointer)

        require_equal(bind_buffer_memory.int_device, unmap_memory.int_device)
        require_equal(bind_buffer_memory.int_memory, unmap_memory.int_memory)
