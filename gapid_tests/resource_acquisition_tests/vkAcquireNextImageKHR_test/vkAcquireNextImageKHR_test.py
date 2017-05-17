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
from gapit_test_framework import GapitTest
import gapit_test_framework
from vulkan_constants import *


@gapit_test("vkAcquireNextImageKHR_test")
class AcquireNextImageWithSemaphoreNoFence(GapitTest):
    def expect(self):
        """1. Expects a call of vkAcquireNextImageKHR() with semaphore and no
        fence."""
        acquire_next_image = require(self.nth_call_of("vkAcquireNextImageKHR",
                                                      1))
        require_equal(acquire_next_image.return_val, VK_SUCCESS)

        require_not_equal(0, acquire_next_image.int_device)
        require_not_equal(0, acquire_next_image.int_swapchain)
        require_equal(10, acquire_next_image.int_timeout)
        require_not_equal(0, acquire_next_image.int_semaphore)
        require_equal(0, acquire_next_image.int_fence)
        require_not_equal(0, acquire_next_image.hex_pImageIndex)

        next_image_index = little_endian_bytes_to_int(require(
            acquire_next_image.get_write_data(
                acquire_next_image.hex_pImageIndex, 4)))
        require_equal(0, next_image_index)
