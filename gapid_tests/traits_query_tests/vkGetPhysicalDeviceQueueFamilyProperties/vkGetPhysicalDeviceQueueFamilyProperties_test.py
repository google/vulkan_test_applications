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
from vulkan_constants import *
from struct_offsets import VulkanStruct, BOOL32, UINT32_T, POINTER, ARRAY

QUEUE_FAMILY_PROPERTIES = [
    ("queueFlags", UINT32_T),
    ("queueCount", UINT32_T),
    ("timestampValidBits", UINT32_T),
    ("minImageTransferGranularity_width", UINT32_T),
    ("minImageTransferGranularity_height", UINT32_T),
    ("minImageTransferGranularity_depth", UINT32_T),
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


@gapit_test("vkGetPhysicalDeviceQueueFamilyProperties_test")
class GetPhysicalDeviceQueueFamilyProperties(GapitTest):

    def expect(self):
        arch = self.architecture
        physical_devices = GetPhysicalDevices(self, arch)
        property_counts = {}

        # case pQueueFamilyProperties == nullptr
        for pd in physical_devices:
            get_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceQueueFamilyProperties"))
            require_equal(pd, get_properties.int_physicalDevice)
            require_not_equal(0, get_properties.hex_pQueueFamilyPropertyCount)
            require_equal(0, get_properties.hex_pQueueFamilyProperties)
            observation_num_properties = little_endian_bytes_to_int(
                require(get_properties.get_write_data(
                    get_properties.hex_pQueueFamilyPropertyCount,
                    arch.int_integerSize)))
            property_counts[pd] = observation_num_properties

        # case *pQueueFamilyPropertyCount == 0
        for pd in physical_devices:
            get_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceQueueFamilyProperties"))
            require_equal(pd, get_properties.int_physicalDevice)
            require_not_equal(0, get_properties.hex_pQueueFamilyPropertyCount)
            require_not_equal(0, get_properties.hex_pQueueFamilyProperties)
            observation_num_properties = little_endian_bytes_to_int(
                require(get_properties.get_write_data(
                    get_properties.hex_pQueueFamilyPropertyCount,
                    arch.int_integerSize)))
            require_equal(0, observation_num_properties)

        # case *pQueueFamilyPropertyCount < capacity
        for pd in physical_devices:
            if property_counts[pd] <= 1:
                continue
            get_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceQueueFamilyProperties"))
            require_equal(pd, get_properties.int_physicalDevice)
            require_not_equal(0, get_properties.hex_pQueueFamilyPropertyCount)
            require_not_equal(0, get_properties.hex_pQueueFamilyProperties)
            # Check the count
            observation_num_properties = little_endian_bytes_to_int(
                require(get_properties.get_write_data(
                    get_properties.hex_pQueueFamilyPropertyCount,
                    arch.int_integerSize)))
            require_equal(property_counts[pd] - 1,
                          observation_num_properties)

            # Check the properties
            offset = 0
            for i in range(property_counts[pd] - 1):
                p = VulkanStruct(
                    arch, QUEUE_FAMILY_PROPERTIES, get_write_offset_function(
                        get_properties,
                        get_properties.hex_pQueueFamilyProperties + offset))
                offset += p.total_size

        # case *pQueueFamilyPropertyCount < capacity
        for pd in physical_devices:
            get_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceQueueFamilyProperties"))
            require_equal(pd, get_properties.int_physicalDevice)
            require_not_equal(0, get_properties.hex_pQueueFamilyPropertyCount)
            require_not_equal(0, get_properties.hex_pQueueFamilyProperties)
            # Check the count
            observation_num_properties = little_endian_bytes_to_int(
                require(get_properties.get_write_data(
                    get_properties.hex_pQueueFamilyPropertyCount,
                    arch.int_integerSize)))
            require_equal(property_counts[pd], observation_num_properties)

            # Check the properties
            offset = 0
            for i in range(property_counts[pd]):
                p = VulkanStruct(
                    arch, QUEUE_FAMILY_PROPERTIES, get_write_offset_function(
                        get_properties,
                        get_properties.hex_pQueueFamilyProperties + offset))
                offset += p.total_size

        # case *pQueueFamilyPropertyCount > capacity
        for pd in physical_devices:
            get_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceQueueFamilyProperties"))
            require_equal(pd, get_properties.int_physicalDevice)
            require_not_equal(0, get_properties.hex_pQueueFamilyPropertyCount)
            require_not_equal(0, get_properties.hex_pQueueFamilyProperties)
            # Check the count
            observation_num_properties = little_endian_bytes_to_int(
                require(get_properties.get_write_data(
                    get_properties.hex_pQueueFamilyPropertyCount,
                    arch.int_integerSize)))
            require_equal(property_counts[pd], observation_num_properties)

            # Check the properties
            offset = 0
            for i in range(property_counts[pd]):
                p = VulkanStruct(
                    arch, QUEUE_FAMILY_PROPERTIES, get_write_offset_function(
                        get_properties,
                        get_properties.hex_pQueueFamilyProperties + offset))
                offset += p.total_size
