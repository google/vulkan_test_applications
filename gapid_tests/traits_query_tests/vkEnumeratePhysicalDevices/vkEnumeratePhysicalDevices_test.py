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
from gapit_test_framework import get_write_offset_function, WINDOWS
import gapit_test_framework
from struct_offsets import VulkanStruct, POINTER, HANDLE, ARRAY
from vulkan_constants import *


@gapit_test("vkEnumeratePhysicalDevices_test")
class EnumeratePhysicalDevices(GapitTest):

    def expect(self):
        """Check the arguments and the values referred by the pointer arguments
        of vkEnumeratePhysicalDevices"""
        architecture = self.architecture

        # The first call to vkEnumeratePhysicalDevices, pPhysicalDevices is
        # NULL, pPhysicalDeviceCount refer to 0 prior to the call and should be
        # non zero after.
        get_count = require(self.next_call_of("vkEnumeratePhysicalDevices"))
        require_not_equal(0, get_count.int_instance)
        require_not_equal(0, get_count.hex_pPhysicalDeviceCount)
        require_equal(0, get_count.hex_pPhysicalDevices)
        # Strictly speaking here the return value should be VK_INCOMPLETE, but
        # Nvidia drivers return VK_SUCCESS when pPhysicalDevices is NULL
        # TODO(qining): Uncomment this once drivers handle it correctly
        #  require_equal(VK_INCOMPLETE, int(get_count.return_val))

        physical_device_count_after_call = require(get_count.get_write_data(
            get_count.hex_pPhysicalDeviceCount, architecture.int_integerSize))
        physical_device_count = little_endian_bytes_to_int(
            physical_device_count_after_call)
        require_not_equal(0, physical_device_count)

        # The second call should have physical device handles returned in the memory
        # pointed by pPhysicalDevices
        get_handles = require(self.next_call_of("vkEnumeratePhysicalDevices"))
        require_not_equal(0, get_handles.int_instance)
        require_not_equal(0, get_handles.hex_pPhysicalDeviceCount)
        require_not_equal(0, get_handles.hex_pPhysicalDevices)
        require_equal(VK_SUCCESS, int(get_handles.return_val))

        # physical device handles are not NON_DISPATCHALBE_HANDLE
        PHYSICAL_DEVICES = [("handles", ARRAY, physical_device_count, POINTER)]
        physical_devices = VulkanStruct(
            architecture, PHYSICAL_DEVICES, get_write_offset_function(
                get_handles, get_handles.hex_pPhysicalDevices))
        for handle in physical_devices.handles:
            require_not_equal(0, handle)

        # The windows loader + some drivers do very odd things here.
        # They change around the calls to EnumeratePhyiscalDevices
        # in order to handle multiple devices.
        if self.device.Configuration.OS.Kind == WINDOWS:
            return
        # The third call is made with non-NULL pPhysicalDevices, while the physical
        # device count is one less than the actual count. Command should return
        # with VK_INCOMPLETE
        one_less_count = require(
            self.next_call_of("vkEnumeratePhysicalDevices"))
        require_not_equal(0, one_less_count.int_instance)
        require_not_equal(0, one_less_count.hex_pPhysicalDeviceCount)
        require_not_equal(0, one_less_count.hex_pPhysicalDevices)
        require_equal(VK_INCOMPLETE, int(one_less_count.return_val))

        # The fourth call is made with non-NULL pPhysicalDevices, while the physical
        # device count is zero. Command should return with VK_INCOMPLETE
        zero_count = require(self.next_call_of("vkEnumeratePhysicalDevices"))
        require_not_equal(0, zero_count.int_instance)
        require_not_equal(0, zero_count.hex_pPhysicalDeviceCount)
        require_not_equal(0, zero_count.hex_pPhysicalDevices)
        require_equal(VK_INCOMPLETE, int(zero_count.return_val))
        physical_device_count_before_call = require(zero_count.get_read_data(
            zero_count.hex_pPhysicalDeviceCount, architecture.int_integerSize))
        require_equal(
            0, little_endian_bytes_to_int(physical_device_count_before_call))
