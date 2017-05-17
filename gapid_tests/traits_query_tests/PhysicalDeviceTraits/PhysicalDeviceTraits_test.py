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

PHYSICAL_DEVICE_FEATURES = [
    ("robustBufferAccess", BOOL32),
    ("fullDrawIndexUint32", BOOL32),
    ("imageCubeArray", BOOL32),
    ("independentBlend", BOOL32),
    ("geometryShader", BOOL32),
    ("tessellationShader", BOOL32),
    ("sampleRateShading", BOOL32),
    ("dualSrcBlend", BOOL32),
    ("logicOp", BOOL32),
    ("multiDrawIndirect", BOOL32),
    ("drawIndirectFirstInstance", BOOL32),
    ("depthClamp", BOOL32),
    ("depthBiasClamp", BOOL32),
    ("fillModeNonSolid", BOOL32),
    ("depthBounds", BOOL32),
    ("wideLines", BOOL32),
    ("largePoints", BOOL32),
    ("alphaToOne", BOOL32),
    ("multiViewport", BOOL32),
    ("samplerAnisotropy", BOOL32),
    ("textureCompressionETC2", BOOL32),
    ("textureCompressionASTC_LDR", BOOL32),
    ("textureCompressionBC", BOOL32),
    ("occlusionQueryPrecise", BOOL32),
    ("pipelineStatisticsQuery", BOOL32),
    ("vertexPipelineStoresAndAtomics", BOOL32),
    ("fragmentStoresAndAtomics", BOOL32),
    ("shaderTessellationAndGeometryPointSize", BOOL32),
    ("shaderImageGatherExtended", BOOL32),
    ("shaderStorageImageExtendedFormats", BOOL32),
    ("shaderStorageImageMultisample", BOOL32),
    ("shaderStorageImageReadWithoutFormat", BOOL32),
    ("shaderStorageImageWriteWithoutFormat", BOOL32),
    ("shaderUniformBufferArrayDynamicIndexing", BOOL32),
    ("shaderSampledImageArrayDynamicIndexing", BOOL32),
    ("shaderStorageBufferArrayDynamicIndexing", BOOL32),
    ("shaderStorageImageArrayDynamicIndexing", BOOL32),
    ("shaderClipDistance", BOOL32),
    ("shaderCullDistance", BOOL32),
    ("shaderFloat64", BOOL32),
    ("shaderInt64", BOOL32),
    ("shaderInt16", BOOL32),
    ("shaderResourceResidency", BOOL32),
    ("shaderResourceMinLod", BOOL32),
    ("sparseBinding", BOOL32),
    ("sparseResidencyBuffer", BOOL32),
    ("sparseResidencyImage2D", BOOL32),
    ("sparseResidencyImage3D", BOOL32),
    ("sparseResidency2Samples", BOOL32),
    ("sparseResidency4Samples", BOOL32),
    ("sparseResidency8Samples", BOOL32),
    ("sparseResidency16Samples", BOOL32),
    ("sparseResidencyAliased", BOOL32),
    ("variableMultisampleRate", BOOL32),
    ("inheritedQueries", BOOL32),
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


@gapit_test("PhysicalDeviceTraits_test")
class GetPhysicalDeviceFeatures(GapitTest):

    def expect(self):
        """1. Expect vkEnumeratePhysicalDevices() to be called first, then
        vkGetPhysicalDeviceFeatures() to be called num_phy_devices times and
        return physical device features."""

        arch = self.architecture
        physical_devices = GetPhysicalDevices(self, arch)
        for pd in physical_devices:
            get_features = require(self.next_call_of(
                "vkGetPhysicalDeviceFeatures"))
            require_equal(pd, get_features.int_physicalDevice)
            require_not_equal(0, get_features.hex_pFeatures)
            VulkanStruct(arch, PHYSICAL_DEVICE_FEATURES,
                         get_write_offset_function(get_features,
                                                   get_features.hex_pFeatures))


@gapit_test("PhysicalDeviceTraits_test")
class GetPhysicalDeviceMemoryProperties(GapitTest):

    def expect(self):
        """2. Expect vkEnumeratePhysicalDevices() to be called first, then
        vkGetPhysicalDeviceMemoryProperties() to be called num_phy_devices times
        and return physical device memory properties."""

        arch = self.architecture
        physical_devices = GetPhysicalDevices(self, arch)
        for pd in physical_devices:
            get_memory_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceMemoryProperties"))
            require_equal(pd, get_memory_properties.int_physicalDevice)
            require_not_equal(0, get_memory_properties.hex_pMemoryProperties)
            # TODO: Check the existance of the memory property struct


@gapit_test("PhysicalDeviceTraits_test")
class GetPhysicalDeviceProperties(GapitTest):

    def expect(self):
        """3. Expect vkEnumeratePhysicalDevices() to be called first, then
        vkGetPhysicalDeviceProperties() to be called num_phy_devices times
        and return physical device properties."""

        arch = self.architecture
        physical_devices = GetPhysicalDevices(self, arch)
        for pd in physical_devices:
            get_device_properties = require(self.next_call_of(
                "vkGetPhysicalDeviceProperties"))
            require_equal(pd, get_device_properties.int_physicalDevice)
            require_not_equal(0, get_device_properties.hex_pProperties)
            # TODO: Check the existance of the property struct
