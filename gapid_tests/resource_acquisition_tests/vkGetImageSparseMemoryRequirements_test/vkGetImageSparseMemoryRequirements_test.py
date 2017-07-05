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
from gapit_test_framework import GapitTest, get_write_offset_function
from gapit_test_framework import get_read_offset_function
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, POINTER, DEVICE_SIZE

SPRASE_IMAGE_MEMORY_REQUIREMENTS = [
    ("formatProperties_aspectMask", UINT32_T),
    ("formatProperties_imageGranularity_width", UINT32_T),
    ("formatProperties_imageGranularity_height", UINT32_T),
    ("formatProperties_imageGranularity_depth", UINT32_T),
    ("formatProperties_flags", UINT32_T),
    ("imageMipTailFirstLod", UINT32_T),
    ("imageMipTailSize", DEVICE_SIZE),
    ("imageMipTailOffset", DEVICE_SIZE),
    ("imageMipTailStride", DEVICE_SIZE),
]


@gapit_test("vkGetImageSparseMemoryRequirements_test")
class GetImageSparseMemoryRequirements(GapitTest):

    def expect(self):
        architecture = self.architecture
        first_call = require(self.next_call_of(
            "vkGetImageSparseMemoryRequirements"))
        require_not_equal(0, first_call.int_device)
        require_not_equal(0, first_call.int_image)
        require_not_equal(0, first_call.hex_pSparseMemoryRequirementCount)
        require_equal(0, first_call.hex_pSparseMemoryRequirements)
        count = little_endian_bytes_to_int(require(first_call.get_write_data(
            first_call.hex_pSparseMemoryRequirementCount,
            architecture.int_integerSize)))

        second_call = require(self.next_call_of(
            "vkGetImageSparseMemoryRequirements"))
        require_not_equal(0, second_call.int_device)
        require_not_equal(0, second_call.int_image)
        require_not_equal(0, second_call.hex_pSparseMemoryRequirementCount)
        require_not_equal(0, second_call.hex_pSparseMemoryRequirements)

        offset = 0
        for i in range(count):
            memory_requirements = VulkanStruct(
                architecture, SPRASE_IMAGE_MEMORY_REQUIREMENTS,
                get_write_offset_function(
                    second_call,
                    second_call.hex_pSparseMemoryRequirements + offset))
            offset += memory_requirements.total_size
