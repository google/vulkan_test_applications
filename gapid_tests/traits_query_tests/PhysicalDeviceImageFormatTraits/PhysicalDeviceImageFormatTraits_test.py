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

from numpy import int32

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_write_offset_function
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, UINT64_T, ARRAY, POINTER

IMAGE_FORMAT_PROPERTIES = [
    ("maxExtent_width", UINT32_T),
    ("maxExtent_height", UINT32_T),
    ("maxExtent_depth", UINT32_T),
    ("maxMipLevels", UINT32_T),
    ("maxArrayLayers", UINT32_T),
    ("maxResourceSize", UINT64_T),
]


def GetPhysicalDevices(test, architecture):
    # first call to enumerate physical devices
    first_enumerate_physical_devices = require(test.next_call_of(
        "vkEnumeratePhysicalDevices"))
    require_equal(VK_SUCCESS, int(first_enumerate_physical_devices.return_val))
    require_not_equal(0, first_enumerate_physical_devices.int_instance)
    require_not_equal(0,
                      first_enumerate_physical_devices.hex_pPhysicalDeviceCount)
    require_equal(0, first_enumerate_physical_devices.hex_pPhysicalDevices)

    num_phy_devices = little_endian_bytes_to_int(require(
        first_enumerate_physical_devices.get_write_data(
            first_enumerate_physical_devices.hex_pPhysicalDeviceCount,
            architecture.int_integerSize)))

    # second call to enumerate physical devices
    second_enumerate_physical_devices = require(test.next_call_of(
        "vkEnumeratePhysicalDevices"))
    require_equal(VK_SUCCESS, int(second_enumerate_physical_devices.return_val))
    require_not_equal(0, second_enumerate_physical_devices.int_instance)
    require_not_equal(
        0, second_enumerate_physical_devices.hex_pPhysicalDeviceCount)
    require_not_equal(0, second_enumerate_physical_devices.hex_pPhysicalDevices)
    require_not_equal(0, num_phy_devices)
    PHYSICAL_DEVICES = [("physicalDevices", ARRAY, num_phy_devices, POINTER)]
    returned_physical_devices = VulkanStruct(
        architecture, PHYSICAL_DEVICES, get_write_offset_function(
            second_enumerate_physical_devices,
            second_enumerate_physical_devices.hex_pPhysicalDevices))
    return returned_physical_devices.physicalDevices


def IsExpectedReturnCode(rc):
    if rc == VK_SUCCESS or rc == VK_ERROR_OUT_OF_HOST_MEMORY or rc == VK_ERROR_OUT_OF_DEVICE_MEMORY or rc == VK_ERROR_FORMAT_NOT_SUPPORTED:
        return True
    return False


def CheckAtom(atom,
              architecture,
              physical_device,
              format=VK_FORMAT_R8G8B8A8_UNORM,
              type=VK_IMAGE_TYPE_2D,
              tiling=VK_IMAGE_TILING_OPTIMAL,
              usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              flags=0):
    require_equal(True, IsExpectedReturnCode(int(int32(atom.return_val))))
    require_equal(physical_device, atom.int_physicalDevice)
    require_equal(format, atom.int_format)
    require_equal(type, atom.int_type)
    require_equal(tiling, atom.int_tiling)
    require_equal(usage, atom.int_usage)
    require_equal(flags, atom.int_flags)
    require_not_equal(0, atom.hex_pImageFormatProperties)

    if int(atom.return_val) == VK_SUCCESS:
        VulkanStruct(architecture, IMAGE_FORMAT_PROPERTIES,
                     get_write_offset_function(atom,
                                               atom.hex_pImageFormatProperties))


@gapit_test("PhysicalDeviceImageFormatTraits_test")
class GetPhysicalDeviceImageFormatProperties(GapitTest):

    def expect(self):
        arch = self.architecture
        physical_devices = GetPhysicalDevices(self, arch)

        for pd in physical_devices:
            for fmt in ALL_VK_FORMATS:
                get_properties = require(self.next_call_of(
                    "vkGetPhysicalDeviceImageFormatProperties"))
                CheckAtom(get_properties, arch, pd, format=fmt)

            for t in ALL_VK_IMAGE_TYPES:
                get_properties = require(self.next_call_of(
                    "vkGetPhysicalDeviceImageFormatProperties"))
                CheckAtom(get_properties, arch, pd, type=t)

            for t in ALL_VK_IMAGE_TILINGS:
                get_properties = require(self.next_call_of(
                    "vkGetPhysicalDeviceImageFormatProperties"))
                CheckAtom(get_properties, arch, pd, tiling=t)

            for u in ALL_VK_IMAGE_USAGE_COMBINATIONS:
                get_properties = require(self.next_call_of(
                    "vkGetPhysicalDeviceImageFormatProperties"))
                CheckAtom(get_properties, arch, pd, usage=u)

            for f in ALL_VK_IMAGE_CREATE_FLAG_COMBINATIONS:
                get_properties = require(self.next_call_of(
                    "vkGetPhysicalDeviceImageFormatProperties"))
                CheckAtom(get_properties, arch, pd, flags=f)
