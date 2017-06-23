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

from gapit_test_framework import gapit_test, GapitTest
from gapit_test_framework import require, require_equal, require_not_equal
from gapit_test_framework import get_write_offset_function
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, UINT64_T, ARRAY

EXTENT_2D = [("width", UINT32_T), ("height", UINT32_T)]


@gapit_test("vkGetRenderAreaGranularity_test")
class GetRenderAreaGranularity(GapitTest):

    def expect(self):
        architecture = self.architecture
        get_granularity = require(self.next_call_of(
            "vkGetRenderAreaGranularity"))

        require_not_equal(0, get_granularity.int_device)
        require_not_equal(0, get_granularity.int_renderPass)
        granularity = VulkanStruct(
            architecture, EXTENT_2D, get_write_offset_function(
                get_granularity, get_granularity.hex_pGranularity))
        require_not_equal(0, granularity.width)
        require_not_equal(0, granularity.height)
