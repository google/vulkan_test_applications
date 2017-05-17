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


@gapit_test("SetDepthBias_test")
class SetDepthBiasWithDepthBiasClampOfValueZero(GapitTest):

    def expect(self):
        DEPTH_BIAS_CONSTANT_FACTOR = 1.1
        DEPTH_BIAS_CLAMP = 0.0
        DEPTH_BIAS_SLOPE_FACTOR = 3.3

        set_depth_bias = require(self.nth_call_of("vkCmdSetDepthBias", 1))

        require_not_equal(0, set_depth_bias.int_commandBuffer)
        require_equal(DEPTH_BIAS_CONSTANT_FACTOR,
                      set_depth_bias.float_depthBiasConstantFactor)
        require_equal(DEPTH_BIAS_CLAMP, set_depth_bias.float_depthBiasClamp)
        require_equal(DEPTH_BIAS_SLOPE_FACTOR,
                      set_depth_bias.float_depthBiasSlopeFactor)


@gapit_test("SetDepthBias_test")
class SetDepthBiasWithDepthBiasClampOfValueNonZero(GapitTest):

    def expect(self):
        DEPTH_BIAS_CONSTANT_FACTOR = 1.1
        DEPTH_BIAS_CLAMP = 2.2
        DEPTH_BIAS_SLOPE_FACTOR = 3.3

        create_device = self.nth_call_of("vkCreateDevice", 2)
        if create_device[0] is None:
            raise GapidUnsupportedException(
                "physical device feature: depthBiasClamp not supported")

        set_depth_bias = require(self.next_call_of("vkCmdSetDepthBias"))

        require_not_equal(0, set_depth_bias.int_commandBuffer)
        require_equal(DEPTH_BIAS_CONSTANT_FACTOR,
                      set_depth_bias.float_depthBiasConstantFactor)
        require_equal(DEPTH_BIAS_CLAMP, set_depth_bias.float_depthBiasClamp)
        require_equal(DEPTH_BIAS_SLOPE_FACTOR,
                      set_depth_bias.float_depthBiasSlopeFactor)
