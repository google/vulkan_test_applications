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


@gapit_test("vkGetSwapchainImagesKHR_test")
class GetSwapchainImages_test(GapitTest):

    def expect(self):
        architecture = self.architecture
        swapchain_images = require(
            self.next_call_of("vkGetSwapchainImagesKHR"))
        require_equal(swapchain_images.return_val, VK_SUCCESS)

        num_swapchain_images = little_endian_bytes_to_int(
            require(
                swapchain_images.get_write_data(
                    swapchain_images.hex_pSwapchainImageCount, 4)))
        require_not_equal(0, num_swapchain_images)

        swapchain_images = require(
            self.next_call_of("vkGetSwapchainImagesKHR"))
        new_num_swapchain_images = little_endian_bytes_to_int(
            require(
                swapchain_images.get_write_data(
                    swapchain_images.hex_pSwapchainImageCount, 4)))
        require_equal(num_swapchain_images, new_num_swapchain_images)

        for i in range(0, num_swapchain_images):
            require(
                swapchain_images.get_write_data(
                    swapchain_images.hex_pSwapchainImages +
                        i * NON_DISPATCHABLE_HANDLE_SIZE,
                    NON_DISPATCHABLE_HANDLE_SIZE))
        require_equal(swapchain_images.return_val, VK_SUCCESS)

        if num_swapchain_images > 1:
            swapchain_images = require(
                self.next_call_of("vkGetSwapchainImagesKHR"))
            for i in range(0, num_swapchain_images - 1):
                require(
                    swapchain_images.get_write_data(
                        swapchain_images.hex_pSwapchainImages +
                            i * NON_DISPATCHABLE_HANDLE_SIZE, NON_DISPATCHABLE_HANDLE_SIZE))
            require_equal(swapchain_images.return_val, VK_INCOMPLETE)
