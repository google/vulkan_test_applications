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
from gapit_test_framework import little_endian_bytes_to_int
from vulkan_constants import *
from struct_offsets import VulkanStruct, ARRAY, FLOAT, UINT32_T


@gapit_test("vkGetDeviceQueue_test")
class GetDeviceQueue(GapitTest):

    def expect(self):
        arch = self.architecture

        create_device = require(self.next_call_of("vkCreateDevice"))
        device = little_endian_bytes_to_int(require(
            create_device.get_write_data(create_device.hex_pDevice,
                                         arch.int_pointerSize)))
        require_equal(VK_SUCCESS, int(create_device.return_val))
        require_not_equal(0, device)

        get_queue = require(self.next_call_of("vkGetDeviceQueue"))
        require_equal(device, get_queue.int_device)
        require_equal(0, get_queue.int_queueFamilyIndex)
        require_equal(0, get_queue.int_queueIndex)
        require_not_equal(0, get_queue.hex_pQueue)

        queue = little_endian_bytes_to_int(require(get_queue.get_write_data(
            get_queue.hex_pQueue, arch.int_pointerSize)))
        require_not_equal(0, queue)
