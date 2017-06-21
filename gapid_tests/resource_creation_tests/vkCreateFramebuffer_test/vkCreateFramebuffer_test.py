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
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *

FRAMEBUFFER_CREATE_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("flags", UINT32_T),
    ("renderPass", HANDLE), ("attachmentCount", UINT32_T),
    ("pAttachments", POINTER), ("width", UINT32_T), ("height", UINT32_T),
    ("layers", UINT32_T)
]


@gapit_test("vkCreateFramebuffer_test")
class SingleAttachment(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        create_framebuffer = require(self.next_call_of("vkCreateFramebuffer"))
        require_not_equal(0, create_framebuffer.int_device)
        require_equal(0, create_framebuffer.hex_pAllocator)

        framebuffer_create_info = VulkanStruct(
            architecture, FRAMEBUFFER_CREATE_INFO,
            get_read_offset_function(create_framebuffer,
                                     create_framebuffer.hex_pCreateInfo))
        require_equal(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                      framebuffer_create_info.sType)
        require_equal(0, framebuffer_create_info.pNext)
        require_equal(0, framebuffer_create_info.flags)
        require_not_equal(0, framebuffer_create_info.renderPass)
        require_equal(1, framebuffer_create_info.attachmentCount)
        require_not_equal(0, framebuffer_create_info.pAttachments)
        require_not_equal(0, framebuffer_create_info.width)
        require_not_equal(0, framebuffer_create_info.height)
        require_equal(1, framebuffer_create_info.layers)

        _ = require(
            create_framebuffer.get_read_data(
                framebuffer_create_info.pAttachments,
                NON_DISPATCHABLE_HANDLE_SIZE))
        _ = require(
            create_framebuffer.get_write_data(
                create_framebuffer.hex_pFramebuffer,
                NON_DISPATCHABLE_HANDLE_SIZE))

        destroy_framebuffer = require(
            self.next_call_of("vkDestroyFramebuffer"))
        require_not_equal(0, destroy_framebuffer.int_framebuffer)
        require_not_equal(0, destroy_framebuffer.int_device)
        require_equal(0, destroy_framebuffer.hex_pAllocator)
