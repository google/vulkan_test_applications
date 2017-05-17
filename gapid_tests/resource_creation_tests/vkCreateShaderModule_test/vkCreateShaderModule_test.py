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
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *


@gapit_test("vkCreateShaderModule_test")
class ShaderModuleTest(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        create_shader_module = require(
            self.next_call_of("vkCreateShaderModule"))
        destroy_shader_module = require(
            self.next_call_of("vkDestroyShaderModule"))

        shader_module_create_info = VulkanStruct(
            architecture, [("sType", UINT32_T), ("pNext", POINTER),
                           ("flags", UINT32_T), ("codeSize", UINT32_T),
                           ("pCode", POINTER)],
            get_read_offset_function(create_shader_module,
                                     create_shader_module.hex_pCreateInfo))

        require_equal(shader_module_create_info.sType,
                      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
        require_equal(shader_module_create_info.pNext, 0)
        require_equal(shader_module_create_info.flags, 0)
        require_not_equal(shader_module_create_info.codeSize, 0)
        require_not_equal(0, shader_module_create_info.pCode)

        _ = require(
            create_shader_module.get_read_data(
                shader_module_create_info.pCode,
                shader_module_create_info.codeSize))

        require_not_equal(0, destroy_shader_module.int_device)
        require_not_equal(0, destroy_shader_module.int_shaderModule)

        # Our second vkDestroySwapchain should have been called with
        # VK_NULL_HANDLE
        destroy_shader_module = require(
            self.next_call_of("vkDestroyShaderModule"))
        require_not_equal(0, destroy_shader_module.int_device)
        require_equal(0, destroy_shader_module.int_shaderModule)
