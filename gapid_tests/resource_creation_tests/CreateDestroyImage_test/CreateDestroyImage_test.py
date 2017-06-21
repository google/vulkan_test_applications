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
from gapit_test_framework import GapitTest
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, POINTER

IMAGE_CREATE_INFO_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("imageType", UINT32_T),
    ("format", UINT32_T),
    ("extent_width", UINT32_T),
    ("extent_height", UINT32_T),
    ("extent_depth", UINT32_T),
    ("mipLevels", UINT32_T),
    ("arrayLayers", UINT32_T),
    ("samples", UINT32_T),
    ("tiling", UINT32_T),
    ("usage", UINT32_T),
    ("sharingMode", UINT32_T),
    ("queueFamilyIndexCount", UINT32_T),
    ("pQueueFamilyIndices", POINTER),
    ("initialLayout", UINT32_T),
]


def check_create_image(test, index):
    """Gets the |index|'th vkCreateImage command call atom, checks its return
    value and arguments. Also checks the image handler value pointed by the
    image pointer. This method does not check the content of the
    VkImageCreateInfo struct pointed by the create info pointer. Returns the
    checked vkCreateImage atom, device handler value and image handler value.
    """
    create_image = require(test.nth_call_of("vkCreateImage", index))
    require_equal(VK_SUCCESS, int(create_image.return_val))
    device = create_image.int_device
    require_not_equal(0, device)
    p_create_info = create_image.hex_pCreateInfo
    require_not_equal(0, p_create_info)
    p_image = create_image.hex_pImage
    require_not_equal(0, p_image)
    image = little_endian_bytes_to_int(require(create_image.get_write_data(
        p_image, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, image)
    return create_image, device, image


def check_destroy_image(test, device, image, device_properties):
    """Checks the |index|'th vkDestroyImage command call atom, including the
    device handler value and the image handler value.
    """
    destroy_image = require(test.next_call_of("vkDestroyImage"))
    require_equal(device, destroy_image.int_device)
    require_equal(image, destroy_image.int_image)


def get_image_create_info(create_image, architecture):
    """Returns a VulkanStruct which is built to represent the image create info
    struct used in the given create image command."""
    def get_data(offset, size):
        return little_endian_bytes_to_int(require(create_image.get_read_data(
            create_image.hex_pCreateInfo + offset, size)))

    return VulkanStruct(architecture, IMAGE_CREATE_INFO_ELEMENTS, get_data)


@gapit_test("CreateDestroyImage_test")
class ColorAttachmentImage(GapitTest):

    def expect(self):
        """1. Expects a normal 2D color attachement image."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 1)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_2D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 32)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)


@gapit_test("CreateDestroyImage_test")
class DepthImage(GapitTest):

    def expect(self):
        """2. Expects a normal depth image."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 2)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_2D)
        require_equal(info.format, VK_FORMAT_D16_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 32)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)


@gapit_test("CreateDestroyImage_test")
class CubeCompatibleAndMutableImage(GapitTest):

    def expect(self):
        """3. Expects a cube compatible image with mutable format support."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 3)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
                      | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
        require_equal(info.imageType, VK_IMAGE_TYPE_2D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 32)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 6)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)


@gapit_test("CreateDestroyImage_test")
class LinearTilingImageWithSrcDstTransferUsage(GapitTest):

    def expect(self):
        """4. Expects an image with linear tiling and can be used as the source
         and destination of a transfer command."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 4)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_2D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 32)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_LINEAR)
        require_equal(info.usage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                      | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)


@gapit_test("CreateDestroyImage_test")
class ColorAttachment3DImage(GapitTest):

    def expect(self):
        """5. Expects a 3D image with extent values and mipLevels value changed.
        """

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 5)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_3D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 8)
        require_equal(info.extent_height, 8)
        require_equal(info.extent_depth, 16)
        require_equal(info.mipLevels, 5)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)


@gapit_test("CreateDestroyImage_test")
class MultiSampledImageWithPreinitializedLayout(GapitTest):

    def expect(self):
        """6. Expects a preinitialized image that support multi-sample."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 6)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_2D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 32)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_4_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_PREINITIALIZED)


@gapit_test("CreateDestroyImage_test")
class ColorAttachment1DImage(GapitTest):

    def expect(self):
        """7. Expects a 1D color attachement image."""

        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))
        create_image, device, image = check_create_image(self, 7)
        info = get_image_create_info(create_image, architecture)
        check_destroy_image(self, device, image, device_properties)

        require_equal(info.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.imageType, VK_IMAGE_TYPE_1D)
        require_equal(info.format, VK_FORMAT_R8G8B8A8_UNORM)
        require_equal(info.extent_width, 32)
        require_equal(info.extent_height, 1)
        require_equal(info.extent_depth, 1)
        require_equal(info.mipLevels, 1)
        require_equal(info.arrayLayers, 1)
        require_equal(info.samples, VK_SAMPLE_COUNT_1_BIT)
        require_equal(info.tiling, VK_IMAGE_TILING_OPTIMAL)
        require_equal(info.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        require_equal(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE)
        require_equal(info.queueFamilyIndexCount, 0)
        require_equal(info.pQueueFamilyIndices, 0)
        require_equal(info.initialLayout, VK_IMAGE_LAYOUT_UNDEFINED)
