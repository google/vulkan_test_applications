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


@gapit_test("PhysicalDeviceSurface_tests")
class PhysicalDeviceSurfaceQueries(GapitTest):

    def expect(self):
        architecture = self.architecture

        enumerate_physical_devices = require(
            self.next_call_of("vkEnumeratePhysicalDevices"))

        num_physical_devices = little_endian_bytes_to_int(
            require(
                enumerate_physical_devices.get_write_data(
                    enumerate_physical_devices.hex_pPhysicalDeviceCount, 4)))
        for i in range(num_physical_devices):
            physical_device_properties = require(
                self.next_call_of("vkGetPhysicalDeviceProperties"))
            get_queue_family_properties = require(
                self.next_call_of("vkGetPhysicalDeviceQueueFamilyProperties"))
            num_queue_families = little_endian_bytes_to_int(
                require(
                    get_queue_family_properties.get_write_data(
                        get_queue_family_properties.
                        hex_pQueueFamilyPropertyCount, 4)))
            supports = False
            for j in range(num_queue_families):
                get_physical_device_surface_support = require(
                    self.next_call_of("vkGetPhysicalDeviceSurfaceSupportKHR"))
                supports_surface = little_endian_bytes_to_int(
                    require(
                        get_physical_device_surface_support.get_write_data(
                            get_physical_device_surface_support.hex_pSupported,
                            4)))
                supports = supports | (supports_surface > 0)

            if supports:
                get_physical_device_surface_capabilities = require(
                    self.next_call_of(
                        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
                require_equal(
                    VK_SUCCESS,
                    int(get_physical_device_surface_capabilities.return_val))
                get_physical_device_surface_formats = require(
                    self.next_call_of("vkGetPhysicalDeviceSurfaceFormatsKHR"))

                get_physical_device_surface_formats_2 = require(
                    self.next_call_of("vkGetPhysicalDeviceSurfaceFormatsKHR"))

                max_surface_format_count = little_endian_bytes_to_int(
                    require(
                        get_physical_device_surface_formats.get_write_data(
                            get_physical_device_surface_formats.
                            hex_pSurfaceFormatCount, 4)))

                if max_surface_format_count > 1:
                    get_physical_device_surface_formats_3 = require(
                        self.next_call_of(
                            "vkGetPhysicalDeviceSurfaceFormatsKHR"))

                    # 2 should request the same number as the return of 1
                    require_equal(
                        max_surface_format_count, little_endian_bytes_to_int(
                            require(
                                get_physical_device_surface_formats_2.
                                get_write_data(
                                    get_physical_device_surface_formats_2.
                                    hex_pSurfaceFormatCount, 4))))

                    # Re-enable this once the loader behaves correctly
                    # require_equal(
                    #     max_surface_format_count - 1,
                    #     little_endian_bytes_to_int(
                    #         require(
                    #             get_physical_device_surface_formats_3.
                    #             get_write_data(
                    #                 get_physical_device_surface_formats_3.
                    #                 hex_pSurfaceFormatCount, 4))))
                # 2 should have observed *PSurfaceFormatCount
                #                            * sizeof(VkSurfaceFormatKHR)
                # = *PSurfaceFormatCount * 4 * 2
                for i in range(0, max_surface_format_count):
                    _ = require(
                        get_physical_device_surface_formats_2.get_write_data(
                            get_physical_device_surface_formats_2.
                            hex_pSurfaceFormats + i * 8, 8))


@gapit_test("PhysicalDeviceSurface_tests")
class PhysicalDevicePresentModeQuery(GapitTest):

    def expect(self):
        architecture = self.architecture

        enumerate_physical_devices = require(
            self.next_call_of("vkEnumeratePhysicalDevices"))

        num_physical_devices = little_endian_bytes_to_int(
            require(
                enumerate_physical_devices.get_write_data(
                    enumerate_physical_devices.hex_pPhysicalDeviceCount, 4)))
        for i in range(num_physical_devices):
            physical_device_properties = require(
                self.next_call_of("vkGetPhysicalDeviceProperties"))
            get_queue_family_properties = require(
                self.next_call_of("vkGetPhysicalDeviceQueueFamilyProperties"))
            num_queue_families = little_endian_bytes_to_int(
                require(
                    get_queue_family_properties.get_write_data(
                        get_queue_family_properties.
                        hex_pQueueFamilyPropertyCount, 4)))
            supports = False
            for j in range(num_queue_families):
                get_physical_device_surface_support = require(
                    self.next_call_of("vkGetPhysicalDeviceSurfaceSupportKHR"))
                supports_surface = little_endian_bytes_to_int(
                    require(
                        get_physical_device_surface_support.get_write_data(
                            get_physical_device_surface_support.hex_pSupported,
                            4)))
                supports = supports | (supports_surface > 0)

            if supports:
                get_physical_device_surface_present_modes = require(
                    self.next_call_of(
                        "vkGetPhysicalDeviceSurfacePresentModesKHR"))

                get_physical_device_surface_present_modes_2 = require(
                    self.next_call_of(
                        "vkGetPhysicalDeviceSurfacePresentModesKHR"))

                max_present_mode_count = little_endian_bytes_to_int(
                    require(
                        get_physical_device_surface_present_modes.
                        get_write_data(
                            get_physical_device_surface_present_modes.
                            hex_pPresentModeCount, 4)))

                if max_present_mode_count > 1:
                    get_physical_device_surface_present_modes_3 = require(
                        self.next_call_of(
                            "vkGetPhysicalDeviceSurfacePresentModesKHR"))

                    # 2 should request the same number as the return of 1
                    require_equal(
                        max_present_mode_count, little_endian_bytes_to_int(
                            require(
                                get_physical_device_surface_present_modes_2.
                                get_write_data(
                                    get_physical_device_surface_present_modes_2.
                                   hex_pPresentModeCount, 4))))

                    # Re-enable this once the loader does this correctly.
                    # require_equal(
                    #    max_present_mode_count - 1,
                    #    little_endian_bytes_to_int(
                    #        require(
                    #            get_physical_device_surface_present_modes_3.
                    #            get_write_data(
                    #                get_physical_device_surface_present_modes_3.
                    #                hex_pPresentModeCount, 4))))
                # 2 should have observed *PSurfaceFormatCount
                #                            * sizeof(uint32_t)
                # = *PSurfaceFormatCount * 4
                for i in range(0, max_present_mode_count):
                    _ = require(
                        get_physical_device_surface_present_modes_2.get_write_data(
                            get_physical_device_surface_present_modes_2.
                            hex_pPresentModes + i * 4, 4))
