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

PIPELINE_CACHE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("initialDataSize", SIZE_T),
    ("pInitialData", POINTER),
]


@gapit_test("PipelineCache_test")
class EmptyPipelineCache(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))

        create_pipeline_cache = require(self.nth_call_of(
            "vkCreatePipelineCache", 1))
        destroy_pipeline_cache = require(self.next_call_of(
            "vkDestroyPipelineCache"))

        pipeline_cache_create_info = VulkanStruct(
            architecture, PIPELINE_CACHE_CREATE_INFO, get_read_offset_function(
                create_pipeline_cache, create_pipeline_cache.hex_pCreateInfo))

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                      pipeline_cache_create_info.sType)
        require_equal(0, pipeline_cache_create_info.pNext)
        require_equal(0, pipeline_cache_create_info.flags)
        require_equal(0, pipeline_cache_create_info.initialDataSize)
        require_equal(0, pipeline_cache_create_info.pInitialData)

        require_not_equal(0, destroy_pipeline_cache.int_device)
        require_not_equal(0, destroy_pipeline_cache.int_pipelineCache)

        if self.not_device(device_properties, 0x5BCE4000, NVIDIA_K2200):
            destroy_pipeline_cache = require(self.next_call_of(
                "vkDestroyPipelineCache"))
            require_not_equal(0, destroy_pipeline_cache.int_device)
            require_equal(0, destroy_pipeline_cache.int_pipelineCache)
