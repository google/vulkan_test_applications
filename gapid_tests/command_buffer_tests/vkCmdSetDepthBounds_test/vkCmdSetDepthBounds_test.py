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
from gapit_test_framework import GapidUnsupportedException
import gapit_test_framework
from vulkan_constants import *


@gapit_test("vkCmdSetDepthBounds_test")
class SetDepthBounds(GapitTest):

    def expect(self):
        MIN_DEPTH_BOUNDS = 0.01
        MAX_DEPTH_BOUNDS = 0.99

        create_device = self.next_call_of("vkCreateDevice")
        if create_device[0] is None:
            raise GapidUnsupportedException(
                "physical device feature: depthBounds not supported")

        set_depth_bounds = require(self.next_call_of("vkCmdSetDepthBounds"))

        require_not_equal(0, set_depth_bounds.int_commandBuffer)
        require_equal(MIN_DEPTH_BOUNDS, set_depth_bounds.float_minDepthBounds)
        require_equal(MAX_DEPTH_BOUNDS, set_depth_bounds.float_maxDepthBounds)
