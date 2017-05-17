
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function
from gapit_test_framework import GapidUnsupportedException
from struct_offsets import *
from vulkan_constants import *

IMAGE_CREATE_INFO_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("imageType", UINT32_T),
    ("format", UINT32_T),
    ("extent_width", UINT32_T),
    ("extent_height", UINT32_T),
    ("extent_depth", UINT32_T),
    ("mipLevels", UINT32_T),
    ("arrayLayers", UINT32_T),
    ("samples", UINT32_T),
    ("tiling", UINT32_T),
    ("usage", UINT32_T),
    ("sharingMode", UINT32_T),
    ("queueFamilyIndexCount", UINT32_T),
    ("pQueueFamilyIndices", POINTER),
    ("initialLayout", UINT32_T),
]

BUFFER_CREATE_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("createFlags", UINT32_T),
    ("size", DEVICE_SIZE), ("usage", UINT32_T), ("sharingMode", UINT32_T),
    ("queueFamilyIndexCount", UINT32_T), ("pQueueFamilyIndices", POINTER)
]


DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("dedicated", BOOL32),
]

DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("dedicated", BOOL32),
]

DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("image", HANDLE),
    ("buffer", HANDLE),
]
VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV = 1000026000
VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV = 1000026001
VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV = 1000026002


MEMORY_ALLOCATE_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("allocationSize", DEVICE_SIZE),
    ("memoryTypeIndex", UINT32_T)
]


@gapit_test("VK_NV_dedicated_allocation_test")
class NV_Dedicated_Allocation_Image(GapitTest):

    def expect(self):
        architecture = self.architecture

        create_device = self.next_call_of("vkCreateDevice")
        if create_device[0] == None:
            raise GapidUnsupportedException(
                "VK_NV_dedicated_allocation not supported")

        create_image = require(self.nth_call_of(
            "vkCreateImage", 2))
        image_create_info = VulkanStruct(
            architecture, IMAGE_CREATE_INFO_ELEMENTS,
            get_read_offset_function(create_image,
                                     create_image.hex_pCreateInfo))

        require_equal(
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, image_create_info.sType)

        nv_image_create_info = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,
            get_read_offset_function(create_image,
                                     image_create_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,
            nv_image_create_info.sType
        )
        require_equal(VK_TRUE, nv_image_create_info.dedicated)

        allocate_memory = require(self.next_call_of(
            "vkAllocateMemory"
        ))

        allocate_memory_info = VulkanStruct(
            architecture, MEMORY_ALLOCATE_INFO,
            get_read_offset_function(allocate_memory,
                                     allocate_memory.hex_pAllocateInfo))

        nv_allocate_memory = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
            get_read_offset_function(allocate_memory,
                                     allocate_memory_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
            nv_allocate_memory.sType
        )
        image = little_endian_bytes_to_int(
            require(create_image.get_write_data(
                create_image.hex_pImage, NON_DISPATCHABLE_HANDLE_SIZE)))

        require_equal(image, nv_allocate_memory.image)


@gapit_test("VK_NV_dedicated_allocation_test")
class NV_Dedicated_Allocation_Buffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_device = self.next_call_of("vkCreateDevice")

        if create_device[0] == None:
            raise GapidUnsupportedException(
                "VK_NV_dedicated_allocation not supported")

        create_buffer = require(self.nth_call_of(
            "vkCreateBuffer", 3))
        buffer_create_info = VulkanStruct(
            architecture, BUFFER_CREATE_INFO,
            get_read_offset_function(create_buffer,
                                     create_buffer.hex_pCreateInfo))

        require_equal(
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, buffer_create_info.sType)

        nv_buffer_create_info = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV,
            get_read_offset_function(create_buffer,
                                     buffer_create_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV,
            nv_buffer_create_info.sType
        )
        require_equal(VK_TRUE, nv_buffer_create_info.dedicated)

        allocate_memory = require(self.next_call_of(
            "vkAllocateMemory"
        ))

        allocate_memory_info = VulkanStruct(
            architecture, MEMORY_ALLOCATE_INFO,
            get_read_offset_function(allocate_memory,
                                     allocate_memory.hex_pAllocateInfo))

        nv_allocate_memory = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
            get_read_offset_function(allocate_memory,
                                     allocate_memory_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
            nv_allocate_memory.sType
        )
        buffer = little_endian_bytes_to_int(
            require(create_buffer.get_write_data(
                create_buffer.hex_pBuffer, NON_DISPATCHABLE_HANDLE_SIZE)))

        require_equal(buffer, nv_allocate_memory.buffer)


@gapit_test("VK_NV_dedicated_allocation_non_dedicated_test")
class NV_Dedicated_Allocation_Image_Non_Dedicated(GapitTest):

    def expect(self):
        architecture = self.architecture

        create_device = self.next_call_of("vkCreateDevice")
        if create_device[0] == None:
            raise GapidUnsupportedException(
                "VK_NV_dedicated_allocation not supported")

        create_image = require(self.nth_call_of(
            "vkCreateImage", 2))
        image_create_info = VulkanStruct(
            architecture, IMAGE_CREATE_INFO_ELEMENTS,
            get_read_offset_function(create_image,
                                     create_image.hex_pCreateInfo))

        require_equal(
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, image_create_info.sType)

        nv_image_create_info = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,
            get_read_offset_function(create_image,
                                     image_create_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,
            nv_image_create_info.sType
        )
        require_equal(VK_FALSE, nv_image_create_info.dedicated)


@gapit_test("VK_NV_dedicated_allocation_non_dedicated_test")
class NV_Dedicated_Allocation_Buffer_Non_Dedicated(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_device = self.next_call_of("vkCreateDevice")

        if create_device[0] == None:
            raise GapidUnsupportedException(
                "VK_NV_dedicated_allocation not supported")

        create_buffer = require(self.nth_call_of(
            "vkCreateBuffer", 3))
        buffer_create_info = VulkanStruct(
            architecture, BUFFER_CREATE_INFO,
            get_read_offset_function(create_buffer,
                                     create_buffer.hex_pCreateInfo))

        require_equal(
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, buffer_create_info.sType)

        nv_buffer_create_info = VulkanStruct(
            architecture, DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV,
            get_read_offset_function(create_buffer,
                                     buffer_create_info.pNext)
        )
        require_equal(
            VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV,
            nv_buffer_create_info.sType
        )
        require_equal(VK_FALSE, nv_buffer_create_info.dedicated)
