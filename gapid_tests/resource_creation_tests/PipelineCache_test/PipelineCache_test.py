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

        require_equal(VK_SUCCESS, int(create_pipeline_cache.return_val))
        require_not_equal(0, create_pipeline_cache.int_device)
        require_not_equal(0, create_pipeline_cache.hex_pCreateInfo)
        require_not_equal(0, create_pipeline_cache.hex_pPipelineCache)
        cache = little_endian_bytes_to_int(require(
            create_pipeline_cache.get_write_data(
                create_pipeline_cache.hex_pPipelineCache,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(0, cache)

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


@gapit_test("PipelineCache_test")
class MergePipelineCaches(GapitTest):

    def expect(self):
        first_create_cache = require(self.nth_call_of("vkCreatePipelineCache",
                                                      2))
        second_create_cache = require(self.next_call_of(
            "vkCreatePipelineCache",))
        third_create_cache = require(self.next_call_of(
            "vkCreatePipelineCache",))

        first_cache = little_endian_bytes_to_int(require(
            first_create_cache.get_write_data(
                first_create_cache.hex_pPipelineCache,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        second_cache = little_endian_bytes_to_int(require(
            second_create_cache.get_write_data(
                second_create_cache.hex_pPipelineCache,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        third_cache = little_endian_bytes_to_int(require(
            third_create_cache.get_write_data(
                third_create_cache.hex_pPipelineCache,
                NON_DISPATCHABLE_HANDLE_SIZE)))

        require_not_equal(0, first_cache)
        require_not_equal(0, second_cache)
        require_not_equal(0, third_cache)

        merge = require(self.next_call_of("vkMergePipelineCaches"))
        require_equal(VK_SUCCESS, int(merge.return_val))
        require_equal(first_cache, merge.int_dstCache)
        require_equal(2, merge.int_srcCacheCount)
        first_src = little_endian_bytes_to_int(require(merge.get_read_data(
            merge.hex_pSrcCaches, NON_DISPATCHABLE_HANDLE_SIZE)))
        second_src = little_endian_bytes_to_int(require(merge.get_read_data(
            merge.hex_pSrcCaches + NON_DISPATCHABLE_HANDLE_SIZE,
            NON_DISPATCHABLE_HANDLE_SIZE)))
        require_equal(second_cache, first_src)
        require_equal(third_cache, second_src)
