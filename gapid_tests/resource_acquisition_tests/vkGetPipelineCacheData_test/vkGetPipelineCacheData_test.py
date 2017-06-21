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

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function, get_write_offset_function
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *

DATA_SIZE = [
    ("dataSize", SIZE_T),
]

DATA_HEADER = [
    ("headerSize", UINT32_T),
    ("headerVersion", UINT32_T),
    ("venderID", UINT32_T),
    ("deviceID", UINT32_T),
    ("cacheID", ARRAY, VK_UUID_SIZE, CHAR),
]

EMPTY_PIPELINE_CACHE_DATA_SIZE = 36


@gapit_test("vkGetPipelineCacheData_test")
class GetDataOfAnEmptyPipelineCache(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))

        get_cache_data_with_null_pdata = require(
            self.nth_call_of("vkGetPipelineCacheData", 1))
        write_data_size = VulkanStruct(
            architecture, DATA_SIZE,
            get_write_offset_function(get_cache_data_with_null_pdata,
                                      get_cache_data_with_null_pdata.hex_pDataSize))
        # The pipeline cache data size must be at least as large as its header
        # size
        require_true(16 + VK_UUID_SIZE <= write_data_size.dataSize)

        get_cache_data_with_not_null_pdata = require(
            self.next_call_of("vkGetPipelineCacheData"))
        # Check the header
        cache_data_header = VulkanStruct(
            architecture, DATA_HEADER,
            get_write_offset_function(get_cache_data_with_not_null_pdata,
                                      get_cache_data_with_not_null_pdata.hex_pData))
        require_equal(16 + VK_UUID_SIZE, cache_data_header.headerSize)
        require_equal(1, cache_data_header.headerVersion)
        require_not_equal(0, cache_data_header.venderID)
        require_not_equal(0, cache_data_header.deviceID)

        destroy_pipeline_cache = require(
            self.next_call_of("vkDestroyPipelineCache"))


@gapit_test("vkGetPipelineCacheData_test")
class GetPipelineCacheDataOfAGraphicsPipeline(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))

        get_cache_data_with_null_pdata = require(
            self.nth_call_of("vkGetPipelineCacheData", 3))
        write_data_size = VulkanStruct(
            architecture, DATA_SIZE,
            get_write_offset_function(get_cache_data_with_null_pdata,
                                      get_cache_data_with_null_pdata.hex_pDataSize))
        # The pipeline cache data size must be at least as large as its header
        # size
        require_true(16 + VK_UUID_SIZE <= write_data_size.dataSize)

        get_cache_data_with_not_null_pdata = require(
            self.next_call_of("vkGetPipelineCacheData"))
        cache_data_header = VulkanStruct(
            architecture, DATA_HEADER,
            get_write_offset_function(get_cache_data_with_not_null_pdata,
                                      get_cache_data_with_not_null_pdata.hex_pData))
        require_equal(16 + VK_UUID_SIZE, cache_data_header.headerSize)
        require_equal(1, cache_data_header.headerVersion)
        require_not_equal(0, cache_data_header.venderID)
        require_not_equal(0, cache_data_header.deviceID)

        destroy_pipeline_cache = require(
            self.next_call_of("vkDestroyPipelineCache"))
