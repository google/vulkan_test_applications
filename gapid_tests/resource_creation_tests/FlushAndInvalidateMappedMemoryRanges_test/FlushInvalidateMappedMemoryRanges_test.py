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
from struct_offsets import VulkanStruct, ARRAY, UINT32_T, DEVICE_SIZE
from struct_offsets import CHAR, POINTER, HANDLE
from vulkan_constants import VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, VK_WHOLE_SIZE

NON_COHERENT_ATOM_SIZE = 256
BUFFER_SIZE = NON_COHERENT_ATOM_SIZE * 2
FLUSH_INVALIDATE_SIZE = NON_COHERENT_ATOM_SIZE
SRC_BUFFER_FLUSH_INVALIDATE_OFFSET = NON_COHERENT_ATOM_SIZE * 2
DST_BUFFER_FLUSH_INVALIDATE_OFFSET = NON_COHERENT_ATOM_SIZE * 5

MAPPED_MEMORY_RANGE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("memory", HANDLE),
    ("offset", DEVICE_SIZE),
    ("size", DEVICE_SIZE),
]


@gapit_test("FlushAndInvalidateMappedMemoryRanges_test")
class FlushMappedMemoryRangesNonZeroOffsetNotWholeSize(GapitTest):

    def expect(self):
        """Check the arguments and the VkMappedMemoryRange structs used in
        vkFlushMappedMemoryRanges(). The flushed memory starts at offset 768 and
        has size 256"""
        MAP_OFFSET = 512
        FLUSH_OFFSET = MAP_OFFSET + 256
        FLUSH_SIZE = 256
        MEMORY_DATA = [("data", ARRAY, FLUSH_SIZE, CHAR)]
        EXPECTED_MEMORY_DATA = [i for i in range(FLUSH_SIZE)]

        architecture = self.architecture
        # The first vkMapMemory() result is managed in VulkanApplication and is
        # not used here, the second is the one we need here.
        map_memory = require(self.nth_call_of("vkMapMemory", 2))
        require_not_equal(0, map_memory.hex_ppData)
        # The flushed data starts at mapped offset + 256
        flushed_data_ptr = little_endian_bytes_to_int(require(
            map_memory.get_write_data(map_memory.hex_ppData,
                                      architecture.int_integerSize))) + 256

        # Check arguments
        flush_mapped_memory_ranges = require(self.nth_call_of(
            "vkFlushMappedMemoryRanges", 1))
        require_equal(1, flush_mapped_memory_ranges.int_memoryRangeCount)
        require_not_equal(0, flush_mapped_memory_ranges.hex_pMemoryRanges)

        # Check the memory range struct content
        mapped_memory_range = VulkanStruct(
            architecture, MAPPED_MEMORY_RANGE, get_read_offset_function(
                flush_mapped_memory_ranges,
                flush_mapped_memory_ranges.hex_pMemoryRanges))
        require_equal(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                      mapped_memory_range.sType)
        require_equal(0, mapped_memory_range.pNext)
        require_not_equal(0, mapped_memory_range.memory)
        require_equal(FLUSH_OFFSET, mapped_memory_range.offset)
        require_equal(FLUSH_SIZE, mapped_memory_range.size)

        # check the flushed memory data
        memory_data = VulkanStruct(
            architecture, MEMORY_DATA, get_read_offset_function(
                flush_mapped_memory_ranges, flushed_data_ptr))
        require_equal(EXPECTED_MEMORY_DATA, memory_data.data)


@gapit_test("FlushAndInvalidateMappedMemoryRanges_test")
class InvalidateMappedMemoryRangesZeroOffsetWholeSize(GapitTest):

    def expect(self):
        """Check the arguments and the VkMappedMemoryRange structs used in
        vkInvalidateMappedMemoryRanges(). The invalidate memory covers the whole
        dst buffer which starts at offset 0 and has size VK_WHOLE_SIZE. Only the
        second half of the buffer has the data copied from the previously
        flushed memory"""
        MAP_OFFSET = 0
        INVALIDATE_OFFSET = MAP_OFFSET
        INVALIDATE_SIZE = VK_WHOLE_SIZE
        DATA_SIZE = 256
        MEMORY_DATA = [("data", ARRAY, DATA_SIZE, CHAR)]
        EXPECTED_MEMORY_DATA = [i for i in range(DATA_SIZE)]

        architecture = self.architecture
        map_memory = require(self.nth_call_of("vkMapMemory", 3))
        require_not_equal(0, map_memory.hex_ppData)
        # The invalidated data offset is equal to the mapped offset, but the
        # flushed data starts at mapped offset + 256
        invalidate_data_ptr = little_endian_bytes_to_int(require(
            map_memory.get_write_data(map_memory.hex_ppData,
                                      architecture.int_integerSize))) + 256

        # Check arguments
        invalidate_mapped_memory_ranges = require(self.nth_call_of(
            "vkInvalidateMappedMemoryRanges", 1))
        require_equal(1, invalidate_mapped_memory_ranges.int_memoryRangeCount)
        require_not_equal(0, invalidate_mapped_memory_ranges.hex_pMemoryRanges)

        # Check the memory range struct contents
        mapped_memory_range = VulkanStruct(
            architecture, MAPPED_MEMORY_RANGE, get_read_offset_function(
                invalidate_mapped_memory_ranges,
                invalidate_mapped_memory_ranges.hex_pMemoryRanges))
        require_equal(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                      mapped_memory_range.sType)
        require_equal(0, mapped_memory_range.pNext)
        require_not_equal(0, mapped_memory_range.memory)
        require_equal(INVALIDATE_OFFSET, mapped_memory_range.offset)
        require_equal(INVALIDATE_SIZE, mapped_memory_range.size)

        # Check data of the invalidated memory
        memory_data = VulkanStruct(
            architecture, MEMORY_DATA, get_write_offset_function(
                invalidate_mapped_memory_ranges, invalidate_data_ptr))
        require_equal(EXPECTED_MEMORY_DATA, memory_data.data)


@gapit_test("FlushAndInvalidateMappedMemoryRanges_test")
class FlushMappedMemoryRangesZeroOffsetWholeSize(GapitTest):

    def expect(self):
        """Check the arguments and the VkMappedMemoryRange structs used in
        vkFlushMappedMemoryRanges(). The flushed memory starts at offset 0 and
        has size VK_WHOLE_SIZE"""
        MAP_OFFSET = 0
        FLUSH_OFFSET = MAP_OFFSET
        FLUSH_SIZE = VK_WHOLE_SIZE
        DATA_SIZE = 512
        MEMORY_DATA = [("data", ARRAY, DATA_SIZE, CHAR)]
        EXPECTED_MEMORY_DATA = [min(i, 512 - i) & 0xFF
                                for i in range(DATA_SIZE)]

        architecture = self.architecture
        # The first vkMapMemory() result is managed in VulkanApplication and is
        # not used here, the second is the one we need here.
        map_memory = require(self.nth_call_of("vkMapMemory", 4))
        require_not_equal(0, map_memory.hex_ppData)
        # The flushed data starts at mapped offset
        flushed_data_ptr = little_endian_bytes_to_int(require(
            map_memory.get_write_data(map_memory.hex_ppData,
                                      architecture.int_integerSize)))

        # Check arguments
        flush_mapped_memory_ranges = require(self.nth_call_of(
            "vkFlushMappedMemoryRanges", 1))
        require_equal(1, flush_mapped_memory_ranges.int_memoryRangeCount)
        require_not_equal(0, flush_mapped_memory_ranges.hex_pMemoryRanges)

        # Check the memory range struct content
        mapped_memory_range = VulkanStruct(
            architecture, MAPPED_MEMORY_RANGE, get_read_offset_function(
                flush_mapped_memory_ranges,
                flush_mapped_memory_ranges.hex_pMemoryRanges))
        require_equal(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                      mapped_memory_range.sType)
        require_equal(0, mapped_memory_range.pNext)
        require_not_equal(0, mapped_memory_range.memory)
        require_equal(FLUSH_OFFSET, mapped_memory_range.offset)
        require_equal(FLUSH_SIZE, mapped_memory_range.size)

        # check the flushed memory data
        memory_data = VulkanStruct(
            architecture, MEMORY_DATA, get_read_offset_function(
                flush_mapped_memory_ranges, flushed_data_ptr))
        require_equal(EXPECTED_MEMORY_DATA, memory_data.data)


@gapit_test("FlushAndInvalidateMappedMemoryRanges_test")
class InvalidateMappedMemoryRangesNonZeroOffsetNotWholeSize(GapitTest):

    def expect(self):
        """Check the arguments and the VkMappedMemoryRange structs used in
        vkInvalidateMappedMemoryRanges(). The invalidate memory covers the
        second half of the dst buffer which starts at offset 1024 and has size
        512. Only the second half of the buffer has the data is invalidated to
        be host accessible."""
        MAP_OFFSET = 1024
        INVALIDATE_OFFSET = MAP_OFFSET + 256
        INVALIDATE_SIZE = 256
        MEMORY_DATA = [("data", ARRAY, INVALIDATE_SIZE, CHAR)]
        EXPECTED_MEMORY_DATA = [(512 - i) & 0xFF
                                for i in range(INVALIDATE_SIZE, 512)]

        architecture = self.architecture
        map_memory = require(self.nth_call_of("vkMapMemory", 5))
        require_not_equal(0, map_memory.hex_ppData)
        # The invalidated data offset is equal to the mapped offset, but the
        # flushed data starts at mapped offset + 256
        invalidate_data_ptr = little_endian_bytes_to_int(require(
            map_memory.get_write_data(map_memory.hex_ppData,
                                      architecture.int_integerSize))) + 256

        # Check arguments
        invalidate_mapped_memory_ranges = require(self.nth_call_of(
            "vkInvalidateMappedMemoryRanges", 1))
        require_equal(1, invalidate_mapped_memory_ranges.int_memoryRangeCount)
        require_not_equal(0, invalidate_mapped_memory_ranges.hex_pMemoryRanges)

        # Check the memory range struct contents
        mapped_memory_range = VulkanStruct(
            architecture, MAPPED_MEMORY_RANGE, get_read_offset_function(
                invalidate_mapped_memory_ranges,
                invalidate_mapped_memory_ranges.hex_pMemoryRanges))
        require_equal(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                      mapped_memory_range.sType)
        require_equal(0, mapped_memory_range.pNext)
        require_not_equal(0, mapped_memory_range.memory)
        require_equal(INVALIDATE_OFFSET, mapped_memory_range.offset)
        require_equal(INVALIDATE_SIZE, mapped_memory_range.size)

        # Check data of the invalidated memory
        memory_data = VulkanStruct(
            architecture, MEMORY_DATA, get_write_offset_function(
                invalidate_mapped_memory_ranges, invalidate_data_ptr))
        require_equal(EXPECTED_MEMORY_DATA, memory_data.data)
