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
from gapit_test_framework import NVIDIA_K2200
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *

PIPELINE_LAYOUT_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("setLayoutCount", UINT32_T),
    ("pSetLayouts", POINTER),
    ("pushConstantRangeCount", UINT32_T),
    ("pPushConstantRanges", POINTER)
]


@gapit_test("vkCreatePipelineLayout_test")
class EmptyLayout(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_pipeline = require(
            self.nth_call_of("vkCreatePipelineLayout", 1))

        require_not_equal(0, create_pipeline.int_device)
        require_not_equal(0, create_pipeline.hex_pCreateInfo)
        require_equal(0, create_pipeline.hex_pAllocator)
        require_not_equal(0, create_pipeline.hex_pPipelineLayout)

        created_pipeline = little_endian_bytes_to_int(
            require(create_pipeline.get_write_data(
                    create_pipeline.hex_pPipelineLayout,
                    NON_DISPATCHABLE_HANDLE_SIZE)))

        pipeline_layout_create_info = VulkanStruct(
            architecture, PIPELINE_LAYOUT_CREATE_INFO,
            get_read_offset_function(create_pipeline,
                                     create_pipeline.hex_pCreateInfo))

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                      pipeline_layout_create_info.sType)
        require_equal(0, pipeline_layout_create_info.pNext)
        require_equal(0, pipeline_layout_create_info.flags)
        require_equal(0, pipeline_layout_create_info.setLayoutCount)
        require_equal(0, pipeline_layout_create_info.pSetLayouts)
        require_equal(0, pipeline_layout_create_info.pushConstantRangeCount)
        require_equal(0, pipeline_layout_create_info.pPushConstantRanges)

        destroy_pipeline = require(
            self.next_call_of("vkDestroyPipelineLayout"))
        require_equal(created_pipeline, destroy_pipeline.int_pipelineLayout)
        require_equal(create_pipeline.int_device, destroy_pipeline.int_device)
        require_equal(0, destroy_pipeline.hex_pAllocator)


@gapit_test("vkCreatePipelineLayout_test")
class SingleLayout(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_pipeline = require(
            self.nth_call_of("vkCreatePipelineLayout", 2))

        require_not_equal(0, create_pipeline.int_device)
        require_not_equal(0, create_pipeline.hex_pCreateInfo)
        require_equal(0, create_pipeline.hex_pAllocator)
        require_not_equal(0, create_pipeline.hex_pPipelineLayout)

        created_pipeline = little_endian_bytes_to_int(require(
            create_pipeline.get_write_data(
                create_pipeline.hex_pPipelineLayout,
                NON_DISPATCHABLE_HANDLE_SIZE)))

        pipeline_layout_create_info = VulkanStruct(
            architecture, PIPELINE_LAYOUT_CREATE_INFO,
            get_read_offset_function(create_pipeline,
                                     create_pipeline.hex_pCreateInfo))

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                      pipeline_layout_create_info.sType)
        require_equal(0, pipeline_layout_create_info.pNext)
        require_equal(0, pipeline_layout_create_info.flags)
        require_equal(1, pipeline_layout_create_info.setLayoutCount)
        require_not_equal(0, pipeline_layout_create_info.pSetLayouts)
        require_equal(0, pipeline_layout_create_info.pushConstantRangeCount)
        require_equal(0, pipeline_layout_create_info.pPushConstantRanges)

        set_layout = little_endian_bytes_to_int(require(
            create_pipeline.get_read_data(
                pipeline_layout_create_info.pSetLayouts,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(VK_NULL_HANDLE, set_layout)

        destroy_pipeline = require(
            self.next_call_of("vkDestroyPipelineLayout"))
        require_equal(created_pipeline, destroy_pipeline.int_pipelineLayout)
        require_equal(create_pipeline.int_device, destroy_pipeline.int_device)
        require_equal(0, destroy_pipeline.hex_pAllocator)


@gapit_test("vkCreatePipelineLayout_test")
class TwoLayouts(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_pipeline = require(
            self.nth_call_of("vkCreatePipelineLayout", 3))

        require_not_equal(0, create_pipeline.int_device)
        require_not_equal(0, create_pipeline.hex_pCreateInfo)
        require_equal(0, create_pipeline.hex_pAllocator)
        require_not_equal(0, create_pipeline.hex_pPipelineLayout)

        created_pipeline = little_endian_bytes_to_int(require(
            create_pipeline.get_write_data(
                create_pipeline.hex_pPipelineLayout,
                NON_DISPATCHABLE_HANDLE_SIZE)))

        pipeline_layout_create_info = VulkanStruct(
            architecture, PIPELINE_LAYOUT_CREATE_INFO,
            get_read_offset_function(create_pipeline,
                                     create_pipeline.hex_pCreateInfo))

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                      pipeline_layout_create_info.sType)
        require_equal(0, pipeline_layout_create_info.pNext)
        require_equal(0, pipeline_layout_create_info.flags)
        require_equal(2, pipeline_layout_create_info.setLayoutCount)
        require_not_equal(0, pipeline_layout_create_info.pSetLayouts)
        require_equal(0, pipeline_layout_create_info.pushConstantRangeCount)
        require_equal(0, pipeline_layout_create_info.pPushConstantRanges)

        _ = require(create_pipeline.get_read_data(
            pipeline_layout_create_info.pSetLayouts,
            NON_DISPATCHABLE_HANDLE_SIZE
        ))

        _ = require(create_pipeline.get_read_data(
            pipeline_layout_create_info.pSetLayouts +
                NON_DISPATCHABLE_HANDLE_SIZE,
            NON_DISPATCHABLE_HANDLE_SIZE
        ))

        set_layout = little_endian_bytes_to_int(require(
            create_pipeline.get_read_data(
                pipeline_layout_create_info.pSetLayouts,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(VK_NULL_HANDLE, set_layout)

        set_layout2 = little_endian_bytes_to_int(require(
            create_pipeline.get_read_data(
                pipeline_layout_create_info.pSetLayouts +
                    NON_DISPATCHABLE_HANDLE_SIZE,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(VK_NULL_HANDLE, set_layout2)

        destroy_pipeline = require(
            self.next_call_of("vkDestroyPipelineLayout"))
        require_equal(created_pipeline, destroy_pipeline.int_pipelineLayout)
        require_equal(create_pipeline.int_device, destroy_pipeline.int_device)
        require_equal(0, destroy_pipeline.hex_pAllocator)


@gapit_test("vkCreatePipelineLayout_test")
class NullDestroy(GapitTest):

    def expect(self):
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        if (self.not_device(device_properties, 0x5BCE4000, NVIDIA_K2200)):
            destroy_pipeline = require(
                self.nth_call_of("vkDestroyPipelineLayout", 4))
            require_equal(0, destroy_pipeline.int_pipelineLayout)
            require_not_equal(0, destroy_pipeline.int_device)
            require_equal(0, destroy_pipeline.hex_pAllocator)
