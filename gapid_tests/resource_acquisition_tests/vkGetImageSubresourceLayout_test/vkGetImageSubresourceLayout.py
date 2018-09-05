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

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal
from gapit_test_framework import GapitTest, get_read_offset_function
from gapit_test_framework import get_write_offset_function
from gapit_test_framework import little_endian_bytes_to_int
from vulkan_constants import *
from struct_offsets import VulkanStruct, ARRAY, FLOAT, UINT32_T, UINT64_T

IMAGE_SUBRESOURCE_ELEMENTS = [
    ("aspectMask", UINT32_T),
    ("mipLevel", UINT32_T),
    ("arrayLayer", UINT32_T),
]

SUBRESOURCE_LAYOUT_ELEMENTS = [
    ("offset", UINT64_T),
    ("size", UINT64_T),
    ("rowPitch", UINT64_T),
    ("arrayPitch", UINT64_T),
    ("depthPitch", UINT64_T),
]


@gapit_test("vkGetImageSubresourceLayout_test")
class GetImageSubResourceLayout(GapitTest):

    def expect(self):
        arch = self.architecture

        get_layout = require(self.next_call_of("vkGetImageSubresourceLayout"))
        require_not_equal(0, get_layout.int_image)
        subresource = VulkanStruct(
            arch, IMAGE_SUBRESOURCE_ELEMENTS,
            get_read_offset_function(get_layout, get_layout.hex_pSubresource))
        layout = VulkanStruct(arch, SUBRESOURCE_LAYOUT_ELEMENTS,
                              get_write_offset_function(get_layout,
                                                        get_layout.hex_pLayout))

        require_equal(VK_IMAGE_ASPECT_COLOR_BIT, subresource.aspectMask)
        require_equal(0, subresource.mipLevel)
        require_equal(0, subresource.arrayLayer)

        require_not_equal(0, layout.size)
        require_not_equal(0, layout.rowPitch)
        # ArrayPitch and DepthPitch may be undefined
