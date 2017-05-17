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
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *


@gapit_test("vkCreateSwapchainKHR_test")
class SwapchainCreateTest(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        createSwapchain = require(self.next_call_of("vkCreateSwapchainKHR"))
        destroySwapchain = require(self.next_call_of("vkDestroySwapchainKHR"))

        def get_swapchain_create_info_member(offset, size):
            return little_endian_bytes_to_int(
                require(
                    createSwapchain.get_read_data(
                        createSwapchain.hex_pCreateInfo + offset, size)))

        swapchain_create_info = VulkanStruct(
            architecture,
            [("sType", UINT32_T),
             ("pNext", POINTER),
             ("flags", UINT32_T),
             ("surface", HANDLE),
             ("minImageCount", UINT32_T),
             ("imageFormat", UINT32_T),
             ("imageColorSpace", UINT32_T),
             ("extent.width", UINT32_T),
             ("extent.height", UINT32_T),
             ("imageArrayLayers", UINT32_T),
             ("imageUsage", UINT32_T),
             ("imageSharingMode", UINT32_T),
             ("queueFamilyIndexCount", UINT32_T),
             ("queueFamilyIndices", POINTER),
             ("preTransform", UINT32_T),
             ("compositeAlpha", UINT32_T),
             ("presentMode", UINT32_T),
             ("clipped", UINT32_T),
             ("oldSwapchain", HANDLE)  # oldSwapchain
             ],
            get_swapchain_create_info_member)

        require_equal(swapchain_create_info.sType,
                      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)

        require_equal(swapchain_create_info.oldSwapchain, 0)
        require_equal(swapchain_create_info.clipped, 0)
        require_equal(swapchain_create_info.imageArrayLayers, 1)
        require_not_equal(0, destroySwapchain.int_swapchain)
        require_equal(True, (swapchain_create_info.queueFamilyIndexCount == 0 or
                             swapchain_create_info.queueFamilyIndexCount == 2))

        # Our second vkDestroySwapchain should have been called with
        # VK_NULL_HANDLE
        destroySwapchain = require(
            self.next_call_of("vkDestroySwapchainKHR"))
        require_equal(0, destroySwapchain.int_swapchain)
