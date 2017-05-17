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
from gapit_test_framework import get_write_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, POINTER, ARRAY, CHAR, UINT32_T
from vulkan_constants import *


@gapit_test("EnumerateLayersAndExtensions")
class EnumerateDeviceExtensionProperties(GapitTest):

    def expect(self):
        """Check the arguments and values referred by the pointer arugments of
        vkEnumerateDeviceExtensionProperties"""
        architecture = self.architecture
        # First call returns the number of extension properties
        get_device_extension_property_count = require(self.next_call_of(
            "vkEnumerateDeviceExtensionProperties"))
        require_equal(VK_SUCCESS,
                      int(get_device_extension_property_count.return_val))
        # Skip the check of layer name because device layer is a deprecate
        # concept, and all layer names will be empty strings.
        require_not_equal(
            0, get_device_extension_property_count.hex_pPropertyCount)
        require_equal(0, get_device_extension_property_count.hex_pProperties)

        property_count_after_call = little_endian_bytes_to_int(require(
            get_device_extension_property_count.get_write_data(
                get_device_extension_property_count.hex_pPropertyCount,
                architecture.int_integerSize)))
        property_count = property_count_after_call

        # If the number of properties is larger than 1, there should be a call
        # with one less number of properties passed to
        # vkEnumerateDeviceExtensionProperties() and a VK_INCOMPLETE should be
        # returned.
        if property_count > 1:
            incomplete_reutrn = require(self.next_call_of(
                "vkEnumerateDeviceExtensionProperties"))
            require_equal(VK_INCOMPLETE, int(incomplete_reutrn.return_val))
            # Skip the check of layer name because device layer is a deprecate
            # concept, and all layer names will be empty strings.
            require_not_equal(0, incomplete_reutrn.hex_pPropertyCount)
            require_not_equal(0, incomplete_reutrn.hex_pProperties)
            one_less_count = little_endian_bytes_to_int(require(
                incomplete_reutrn.get_write_data(
                    incomplete_reutrn.hex_pPropertyCount,
                    architecture.int_integerSize)))
            require_equal(one_less_count, property_count - 1)

        # If the number of properties is not zero, there must be a call to get
        # the properties, and each property struct should have a non-zero
        # specVersion value.
        if property_count > 0:
            get_device_extension_properties = require(self.next_call_of(
                "vkEnumerateDeviceExtensionProperties"))
            require_equal(VK_SUCCESS,
                          int(get_device_extension_properties.return_val))
            # Skip the check of layer name because device layer is a deprecate
            # concept, and all layer names will be empty strings.
            require_not_equal(
                0, get_device_extension_properties.hex_pPropertyCount)
            require_not_equal(0,
                              get_device_extension_properties.hex_pProperties)
            property_count_after_call = little_endian_bytes_to_int(require(
                get_device_extension_properties.get_write_data(
                    get_device_extension_properties.hex_pPropertyCount,
                    architecture.int_integerSize)))
            require_equal(property_count, property_count_after_call)
            EXTENSION_PROPERTIES = [
                ("extensionName", ARRAY, VK_MAX_EXTENSION_NAME_SIZE, CHAR),
                ("specVersion", UINT32_T),
            ]
            property_offset = 0
            for i in range(property_count_after_call):
                property = VulkanStruct(
                    architecture, EXTENSION_PROPERTIES,
                    get_write_offset_function(
                        get_device_extension_properties,
                        get_device_extension_properties.hex_pProperties +
                        property_offset))
                property_offset += property.total_size
                require_not_equal(0, property.specVersion)
