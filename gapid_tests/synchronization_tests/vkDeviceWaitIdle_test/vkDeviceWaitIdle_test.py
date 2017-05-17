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
from gapit_test_framework import GapitTest, require_not_equal
from vulkan_constants import VK_SUCCESS


@gapit_test("vkDeviceWaitIdle_test")
class WaitForSingleQueue(GapitTest):

    def expect(self):

        device_wait_idle = require(self.nth_call_of("vkDeviceWaitIdle", 1))

        require_not_equal(0, device_wait_idle.int_device)
        require_equal(VK_SUCCESS, int(device_wait_idle.return_val))
