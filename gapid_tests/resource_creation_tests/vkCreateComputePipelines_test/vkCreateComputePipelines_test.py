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

from gapit_test_framework import gapit_test, GapitTest
from gapit_test_framework import require, require_equal, require_not_equal
from gapit_test_framework import get_read_offset_function
from struct_offsets import VulkanStruct, UINT32_T, POINTER, HANDLE
from vulkan_constants import *

COMPUTE_PIPELINE_CREATE_INFO_32 = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("__pad0__", UINT32_T),
    # Start expanding VkPipelineShaderStageCreateInfo
    ("stage_sType", UINT32_T),
    ("stage_pNext", POINTER),
    ("stage_flags", UINT32_T),
    ("stage_stage", UINT32_T),
    ("stage_module", HANDLE),
    ("stage_pName", POINTER),
    ("stage_pSpecializationInfo", POINTER),
    # End
    ("layout", HANDLE),
    ("basePipelineHandle", HANDLE),
    ("basePipelineIndex", UINT32_T),
]


@gapit_test("vkCreateComputePipelines_test")
class DoubleNumbers(GapitTest):

    def expect(self):
        arch = self.architecture

        create_compute_pipelines = require(
            self.next_call_of("vkCreateComputePipelines"))
        require_not_equal(0, create_compute_pipelines.int_device)
        require_equal(0, create_compute_pipelines.int_pipelineCache)
        require_equal(1, create_compute_pipelines.int_createInfoCount)
        require_not_equal(0, create_compute_pipelines.hex_pCreateInfos)
        require_equal(0, create_compute_pipelines.hex_pAllocator)
        require_not_equal(0, create_compute_pipelines.hex_pPipelines)

        self.check_create_info(arch, create_compute_pipelines,
                               COMPUTE_PIPELINE_CREATE_INFO_32)

    def check_create_info(self, arch, atom, create_info_scheme):
        create_info = VulkanStruct(
            arch, create_info_scheme,
            get_read_offset_function(atom, atom.hex_pCreateInfos))

        require_equal(VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                      create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(0, create_info.flags)

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                      create_info.stage_sType)
        require_equal(0, create_info.stage_pNext)
        require_equal(0, create_info.stage_flags)
        require_equal(VK_SHADER_STAGE_COMPUTE_BIT, create_info.stage_stage)
        require_not_equal(0, create_info.stage_module)
        require_equal("main",
                      require(atom.get_read_string(create_info.stage_pName)))
        require_equal(0, create_info.stage_pSpecializationInfo)

        require_not_equal(0, create_info.layout)
        require_equal(0, create_info.basePipelineHandle)
        require_equal(0, create_info.basePipelineIndex)
