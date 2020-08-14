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
from struct_offsets import *
from vulkan_constants import *

GRAPHICS_PIPELINE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("stageCount", UINT32_T),
    ("pStages", POINTER),
    ("pVertexInputState", POINTER),
    ("pInputAssemblyState", POINTER),
    ("pTessellationState", POINTER),
    ("pViewportState", POINTER),
    ("pRasterizationState", POINTER),
    ("pMultisampleState", POINTER),
    ("pDepthStencilState", POINTER),
    ("pColorBlendState", POINTER),
    ("pDynamicState", POINTER),
    ("layout", POINTER),
    ("renderPass", HANDLE),
    ("subpass", UINT32_T),
    ("basePipelineHandle", HANDLE),
    ("basePipelineIndex", INT32_T)
]

PIPELINE_SHADER_STAGE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("stage", UINT32_T),
    ("module", HANDLE),
    ("pName", POINTER),
    ("pSpecializationInfo", POINTER)
]

SPECIALIZATION_INFO = [
    ("mapEntryCount", UINT32_T),
    ("pMapEntries", POINTER),
    ("dataSize", SIZE_T),
    ("pData", POINTER)
]

VERTEX_INPUT_STATE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("vertexBindingDescriptionCount", UINT32_T),
    ("pVertexBindingDescriptions", POINTER),
    ("vertexAttributeDescriptionCount", UINT32_T),
    ("pVertexAttributeDescriptions", POINTER)
]

VERTEX_INPUT_BINDING_DESCRIPTION = [
    ("binding", UINT32_T),
    ("stride", UINT32_T),
    ("inputRate", UINT32_T)
]

VERTEX_INPUT_ATTRIBUTE_DESCRIPTION = [
    ("location", UINT32_T),
    ("binding", UINT32_T),
    ("format", UINT32_T),
    ("offset", UINT32_T)
]

PIPELINE_TESSELLATION_STATE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("patchControlPoints", UINT32_T)
]

INPUT_ASSEMBLY_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("topology", UINT32_T),
    ("primitiveRestartEnable", BOOL32)
]

VIEWPORT_STATE_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("viewportCount", UINT32_T),
    ("pViewports", POINTER),
    ("scissorCount", UINT32_T),
    ("pScissors", POINTER)
]

VIEWPORT = [
    ("x", FLOAT),
    ("y", FLOAT),
    ("width", FLOAT),
    ("height", FLOAT),
    ("minDepth", FLOAT),
    ("maxDepth", FLOAT)
]

RECT2D = [
    ("offset_x", INT32_T),
    ("offset_y", INT32_T),
    ("extent_width", INT32_T),
    ("extent_height", INT32_T)
]

PIPELINE_RASERIZATION_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("depthClampEnable", BOOL32),
    ("rasterizerDiscardEnable", BOOL32),
    ("polygonMode", UINT32_T),
    ("cullMode", UINT32_T),
    ("frontFace", UINT32_T),
    ("depthBiasEnable", BOOL32),
    ("depthBiasConstantFactor", FLOAT),
    ("depthBiasClamp", FLOAT),
    ("depthBiasSlopeFactor", FLOAT),
    ("lineWidth", FLOAT)
]

MULTISAMPLE_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("rasterizationSamples", UINT32_T),
    ("sampleShadingEnable", BOOL32),
    ("minSampleShading", FLOAT),
    ("pSampleMask", POINTER),
    ("alphaToCoverageEnable", BOOL32),
    ("alphaToOneEnable", BOOL32)
]

DEPTH_STENCIL_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("depthTestEnable", BOOL32),
    ("depthWriteEnable", BOOL32),
    ("depthCompareOp", UINT32_T),
    ("depthBoundsTestEnable", BOOL32),
    ("stencilTestEnable", BOOL32),
    ("front_failOp", UINT32_T),
    ("front_passOp", UINT32_T),
    ("front_depthFailOp", UINT32_T),
    ("front_compareOp", UINT32_T),
    ("front_compareMask", UINT32_T),
    ("front_writeMask", UINT32_T),
    ("front_reference", UINT32_T),
    ("back_failOp", UINT32_T),
    ("back_passOp", UINT32_T),
    ("back_depthFailOp", UINT32_T),
    ("back_compareOp", UINT32_T),
    ("back_compareMask", UINT32_T),
    ("back_writeMask", UINT32_T),
    ("back_reference", UINT32_T),
    ("minDepthBounds", FLOAT),
    ("maxDepthBounds", FLOAT)
]

PIPELINE_COLOR_BLEND_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("logicOpEnable", BOOL32),
    ("logicOp", UINT32_T),
    ("attachmentCount", UINT32_T),
    ("pAttachments", POINTER),
    ("blendConstant0", FLOAT),
    ("blendConstant1", FLOAT),
    ("blendConstant2", FLOAT),
    ("blendConstant3", FLOAT)
]

PIPELINE_COLOR_BLEND_ATTACHMENT_STATE = [
    ("blendEnable", BOOL32),
    ("srcColorBlendFactor", UINT32_T),
    ("dstColorBlendFactor", UINT32_T),
    ("colorBlendOp", UINT32_T),
    ("srcAlphaBlendFactor", UINT32_T),
    ("dstAlphaBlendFactor", UINT32_T),
    ("alphaBlendOp", UINT32_T),
    ("colorWriteMask", UINT32_T)
]

DYNAMIC_STATE = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("dynamicStateCount", UINT32_T),
    ("pDynamicStates", POINTER)
]

@gapit_test("vkCreateGraphicsPipelines_test")
class SimpleVertexFragment(GapitTest):

    def expect(self):
        architecture = self.architecture
        create_graphics_pipelines = require(self.next_call_of(
            "vkCreateGraphicsPipelines"))
        require_not_equal(0, create_graphics_pipelines.int_device)
        require_not_equal(0, create_graphics_pipelines.int_pipelineCache)
        require_equal(1, create_graphics_pipelines.int_createInfoCount)
        require_not_equal(0, create_graphics_pipelines.hex_pCreateInfos)
        require_equal(0, create_graphics_pipelines.hex_pAllocator)
        require_not_equal(0, create_graphics_pipelines.hex_pPipelines)

        create_info = VulkanStruct(
            architecture, GRAPHICS_PIPELINE_CREATE_INFO,
            get_read_offset_function(create_graphics_pipelines,
                                     create_graphics_pipelines.hex_pCreateInfos))
        require_equal(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(0, create_info.flags)
        require_equal(2, create_info.stageCount)
        require_not_equal(0, create_info.pStages)
        require_not_equal(0, create_info.pVertexInputState)
        require_not_equal(0, create_info.pInputAssemblyState)
        require_equal(0, create_info.pTessellationState)
        require_not_equal(0, create_info.pViewportState)
        require_not_equal(0, create_info.pRasterizationState)
        require_not_equal(0, create_info.pMultisampleState)
        require_not_equal(0, create_info.pDepthStencilState)
        require_not_equal(0, create_info.pColorBlendState)
        require_equal(0, create_info.pDynamicState)
        require_not_equal(0, create_info.layout)
        require_not_equal(0, create_info.renderPass)
        require_equal(0, create_info.subpass)
        require_equal(0, create_info.basePipelineHandle)
        require_equal(0, create_info.basePipelineIndex)

        shader_stage_create_info_0 = VulkanStruct(
            architecture, PIPELINE_SHADER_STAGE_CREATE_INFO,
            get_read_offset_function(create_graphics_pipelines,
                                     create_info.pStages))
        require_equal(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            shader_stage_create_info_0.sType)
        require_equal(0, shader_stage_create_info_0.pNext)
        require_equal(0, shader_stage_create_info_0.flags)
        require_equal(VK_SHADER_STAGE_VERTEX_BIT,
            shader_stage_create_info_0.stage)
        require_not_equal(0, shader_stage_create_info_0.module)
        require_equal("main", require(
            create_graphics_pipelines.get_read_string(
                shader_stage_create_info_0.pName)))
        require_equal(0, shader_stage_create_info_0.pSpecializationInfo)

        shader_stage_create_info_1 = VulkanStruct(
            architecture, PIPELINE_SHADER_STAGE_CREATE_INFO,
            get_read_offset_function(create_graphics_pipelines,
                                     create_info.pStages +
                                     shader_stage_create_info_0.total_size))

        require_equal(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            shader_stage_create_info_1.sType)
        require_equal(0, shader_stage_create_info_1.pNext)
        require_equal(0, shader_stage_create_info_1.flags)
        require_equal(VK_SHADER_STAGE_FRAGMENT_BIT,
            shader_stage_create_info_1.stage)
        require_not_equal(0, shader_stage_create_info_1.module)
        require_equal("main", require(
            create_graphics_pipelines.get_read_string(
                shader_stage_create_info_1.pName)))
        require_equal(0, shader_stage_create_info_1.pSpecializationInfo)

        vertex_input_state = VulkanStruct(
            architecture, VERTEX_INPUT_STATE_CREATE_INFO,
            get_read_offset_function(create_graphics_pipelines,
                                     create_info.pVertexInputState))
        require_equal(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            vertex_input_state.sType)
        require_equal(0, vertex_input_state.pNext)
        require_equal(0, vertex_input_state.flags)
        require_equal(1, vertex_input_state.vertexBindingDescriptionCount)
        require_not_equal(0, vertex_input_state.pVertexBindingDescriptions)
        require_equal(2, vertex_input_state.vertexAttributeDescriptionCount)
        require_not_equal(0, vertex_input_state.pVertexAttributeDescriptions)

        vertex_binding_description = VulkanStruct(
            architecture, VERTEX_INPUT_BINDING_DESCRIPTION,
                get_read_offset_function(
                    create_graphics_pipelines,
                    vertex_input_state.pVertexBindingDescriptions))
        require_equal(0, vertex_binding_description.binding)
        require_equal(24, vertex_binding_description.stride)
        require_equal(VK_VERTEX_INPUT_RATE_VERTEX,
            vertex_binding_description.inputRate)

        vertex_attribute_description_0 = VulkanStruct(
            architecture, VERTEX_INPUT_ATTRIBUTE_DESCRIPTION,
                get_read_offset_function(
                    create_graphics_pipelines,
                    vertex_input_state.pVertexAttributeDescriptions))

        require_equal(0, vertex_attribute_description_0.location)
        require_equal(0, vertex_attribute_description_0.binding)
        require_equal(VK_FORMAT_R32G32B32A32_SFLOAT,
            vertex_attribute_description_0.format)
        require_equal(0, vertex_attribute_description_0.offset)

        vertex_attribute_description_1 = VulkanStruct(
            architecture, VERTEX_INPUT_ATTRIBUTE_DESCRIPTION,
                get_read_offset_function(
                    create_graphics_pipelines,
                    vertex_input_state.pVertexAttributeDescriptions +
                    vertex_attribute_description_0.total_size))
        require_equal(1, vertex_attribute_description_1.location)
        require_equal(0, vertex_attribute_description_1.binding)
        require_equal(VK_FORMAT_R32G32_SFLOAT,
            vertex_attribute_description_1.format)
        require_equal(16, vertex_attribute_description_1.offset)

        input_assembly_state = VulkanStruct(
            architecture, INPUT_ASSEMBLY_STATE,
            get_read_offset_function(create_graphics_pipelines,
            create_info.pInputAssemblyState))

        require_equal(
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            input_assembly_state.sType)
        require_equal(0, input_assembly_state.pNext)
        require_equal(0, input_assembly_state.flags)
        require_equal(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            input_assembly_state.topology)
        require_equal(0, input_assembly_state.primitiveRestartEnable)

        viewport_state = VulkanStruct(
            architecture, VIEWPORT_STATE_CREATE_INFO,
            get_read_offset_function(create_graphics_pipelines,
                create_info.pViewportState))
        require_equal(
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            viewport_state.sType
        )
        require_equal(0, viewport_state.pNext)
        require_equal(0, viewport_state.flags)
        require_equal(1, viewport_state.viewportCount)
        require_not_equal(0, viewport_state.pViewports)
        require_equal(1, viewport_state.scissorCount)
        require_not_equal(0, viewport_state.pScissors)

        viewport = VulkanStruct(
            architecture, VIEWPORT,
            get_read_offset_function(create_graphics_pipelines,
            viewport_state.pViewports))
        require_equal(0.0, viewport.x)
        require_equal(0.0, viewport.y)
        require_not_equal(0.0, viewport.width)
        require_not_equal(0.0, viewport.height)
        require_equal(0.0, viewport.minDepth)
        require_equal(1.0, viewport.maxDepth)

        scissor = VulkanStruct(
            architecture, RECT2D,
            get_read_offset_function(create_graphics_pipelines,
            viewport_state.pScissors))
        require_equal(0, scissor.offset_x)
        require_equal(0, scissor.offset_y)
        require_not_equal(0, scissor.extent_width)
        require_not_equal(0, scissor.extent_height)

        rasterization_state = VulkanStruct(
            architecture, PIPELINE_RASERIZATION_STATE,
            get_read_offset_function(create_graphics_pipelines,
                create_info.pRasterizationState))
        require_equal(
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            rasterization_state.sType)
        require_equal(0, rasterization_state.pNext)
        require_equal(0, rasterization_state.flags)
        require_equal(0, rasterization_state.depthClampEnable)
        require_equal(0, rasterization_state.rasterizerDiscardEnable)
        require_equal(VK_POLYGON_MODE_FILL, rasterization_state.polygonMode)
        require_equal(VK_CULL_MODE_BACK_BIT, rasterization_state.cullMode)
        require_equal(VK_FRONT_FACE_CLOCKWISE, rasterization_state.frontFace)
        require_equal(0, rasterization_state.depthBiasEnable)
        require_equal(0.0, rasterization_state.depthBiasConstantFactor)
        require_equal(0.0, rasterization_state.depthBiasClamp)
        require_equal(0.0, rasterization_state.depthBiasSlopeFactor)
        require_equal(1.0, rasterization_state.lineWidth)

        multisample_state = VulkanStruct(architecture,
            MULTISAMPLE_STATE, get_read_offset_function(
                create_graphics_pipelines,
                create_info.pMultisampleState))
        require_equal(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            multisample_state.sType)
        require_equal(0, multisample_state.pNext)
        require_equal(0, multisample_state.flags)
        require_equal(VK_SAMPLE_COUNT_1_BIT,
            multisample_state.rasterizationSamples)
        require_equal(0, multisample_state.sampleShadingEnable)
        require_equal(0.0, multisample_state.minSampleShading)
        require_equal(0, multisample_state.pSampleMask)
        require_equal(0, multisample_state.alphaToCoverageEnable)
        require_equal(0, multisample_state.alphaToOneEnable)

        depth_stencil_state = VulkanStruct(architecture,
            DEPTH_STENCIL_STATE, get_read_offset_function(
                create_graphics_pipelines,
                create_info.pDepthStencilState))
        require_equal(
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            depth_stencil_state.sType)
        require_equal(0, depth_stencil_state.pNext)
        require_equal(0, depth_stencil_state.flags)
        require_equal(1, depth_stencil_state.depthTestEnable)
        require_equal(1, depth_stencil_state.depthWriteEnable)
        require_equal(VK_COMPARE_OP_LESS, depth_stencil_state.depthCompareOp)
        require_equal(0, depth_stencil_state.depthBoundsTestEnable)
        require_equal(0, depth_stencil_state.stencilTestEnable)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.front_failOp)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.front_passOp)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.front_depthFailOp)
        require_equal(VK_COMPARE_OP_NEVER, depth_stencil_state.front_compareOp)
        require_equal(0, depth_stencil_state.front_compareMask)
        require_equal(0, depth_stencil_state.front_writeMask)
        require_equal(0, depth_stencil_state.front_reference)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.back_failOp)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.back_passOp)
        require_equal(VK_STENCIL_OP_KEEP, depth_stencil_state.back_depthFailOp)
        require_equal(VK_COMPARE_OP_NEVER, depth_stencil_state.back_compareOp)
        require_equal(0, depth_stencil_state.back_compareMask)
        require_equal(0, depth_stencil_state.back_writeMask)
        require_equal(0, depth_stencil_state.back_reference)
        require_equal(0.0, depth_stencil_state.minDepthBounds)
        require_equal(1.0, depth_stencil_state.maxDepthBounds)

        color_blend_state = VulkanStruct(architecture,
            PIPELINE_COLOR_BLEND_STATE, get_read_offset_function(
                create_graphics_pipelines,
                create_info.pColorBlendState))
        require_equal(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            color_blend_state.sType)
        require_equal(0, color_blend_state.pNext)
        require_equal(0, color_blend_state.flags)
        require_equal(0, color_blend_state.logicOpEnable)
        require_equal(VK_LOGIC_OP_CLEAR, color_blend_state.logicOp)
        require_equal(1, color_blend_state.attachmentCount)
        require_not_equal(0, color_blend_state.pAttachments)
        require_equal(1.0, color_blend_state.blendConstant0)
        require_equal(1.0, color_blend_state.blendConstant1)
        require_equal(1.0, color_blend_state.blendConstant2)
        require_equal(1.0, color_blend_state.blendConstant3)

        color_blend_attachment_state = VulkanStruct(
            architecture, PIPELINE_COLOR_BLEND_ATTACHMENT_STATE,
            get_read_offset_function(
                    create_graphics_pipelines, color_blend_state.pAttachments))
        require_equal(0, color_blend_attachment_state.blendEnable)
        require_equal(VK_BLEND_FACTOR_ZERO,
            color_blend_attachment_state.srcColorBlendFactor)
        require_equal(VK_BLEND_FACTOR_ONE,
            color_blend_attachment_state.dstColorBlendFactor)
        require_equal(VK_BLEND_OP_ADD,
            color_blend_attachment_state.colorBlendOp)
        require_equal(VK_BLEND_FACTOR_ZERO,
            color_blend_attachment_state.srcAlphaBlendFactor)
        require_equal(VK_BLEND_FACTOR_ONE,
            color_blend_attachment_state.dstAlphaBlendFactor)
        require_equal(VK_BLEND_OP_ADD,
            color_blend_attachment_state.alphaBlendOp)

        require_equal(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            color_blend_attachment_state.colorWriteMask)

        destroy_pipeline = require(self.next_call_of(
            "vkDestroyPipeline"))
        require_equal(create_graphics_pipelines.int_device,
            destroy_pipeline.int_device)
        require_equal(0, destroy_pipeline.hex_pAllocator)

        created_pipeline = little_endian_bytes_to_int(require(
            create_graphics_pipelines.get_write_data(
                create_graphics_pipelines.hex_pPipelines,
                NON_DISPATCHABLE_HANDLE_SIZE)))
        require_equal(created_pipeline, destroy_pipeline.int_pipeline)
