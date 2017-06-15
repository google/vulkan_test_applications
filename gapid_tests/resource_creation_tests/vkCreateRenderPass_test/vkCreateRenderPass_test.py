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

RENDER_PASS_CREATE_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("flags", UINT32_T),
    ("attachmentCount", UINT32_T), ("pAttachments", POINTER),
    ("subpassCount", UINT32_T), ("pSubpasses", POINTER),
    ("dependencyCount", UINT32_T), ("pDependencies", POINTER)
]

ATTACHMENT_DESCRIPTION = [
    ("flags", UINT32_T), ("format", UINT32_T), ("samples", UINT32_T),
    ("loadOp", UINT32_T), ("storeOp", UINT32_T), ("stencilLoadOp", UINT32_T),
    ("stencilStoreOp", UINT32_T), ("initialLayout", UINT32_T),
    ("finalLayout", UINT32_T)
]

SUBPASS_DESCRIPTION = [
    ("flags", UINT32_T), ("pipelineBindPoint", UINT32_T),
    ("inputAttachmentCount", UINT32_T), ("pInputAttachments", POINTER),
    ("colorAttachmentCount", UINT32_T), ("pColorAttachments", POINTER),
    ("pResolveAttachments", POINTER), ("pDepthStencilAttachment", POINTER),
    ("preserveAttachmentCount", UINT32_T), ("pPreserveAttachments", POINTER)
]

ATTACHMENT_REFERENCE = [("attachment", UINT32_T), ("layout", UINT32_T)]


@gapit_test("vkCreateRenderPass_test")
class EmptyPass(GapitTest):

    def expect(self):
        architecture = self.architecture

        create_render_pass = require(self.nth_call_of("vkCreateRenderPass", 1))
        destroy_render_pass = require(self.next_call_of("vkDestroyRenderPass"))

        render_pass_create_info = VulkanStruct(
            architecture, RENDER_PASS_CREATE_INFO,
            get_read_offset_function(create_render_pass,
                                     create_render_pass.hex_pCreateInfo))

        subpass_info = VulkanStruct(architecture, SUBPASS_DESCRIPTION,
                                    get_read_offset_function(
                                        create_render_pass,
                                        render_pass_create_info.pSubpasses))

        require_equal(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                      render_pass_create_info.sType)
        require_equal(0, render_pass_create_info.pNext)
        require_equal(0, render_pass_create_info.flags)
        require_equal(0, render_pass_create_info.attachmentCount)
        require_equal(0, render_pass_create_info.pAttachments)
        require_equal(1, render_pass_create_info.subpassCount)
        require_not_equal(0, render_pass_create_info.pSubpasses)
        require_equal(0, render_pass_create_info.dependencyCount)
        require_equal(0, render_pass_create_info.pDependencies)

        require_equal(0, subpass_info.flags)
        require_equal(VK_PIPELINE_BIND_POINT_GRAPHICS,
                      subpass_info.pipelineBindPoint)
        require_equal(0, subpass_info.inputAttachmentCount)
        require_equal(0, subpass_info.pInputAttachments)
        require_equal(0, subpass_info.colorAttachmentCount)
        require_equal(0, subpass_info.pColorAttachments)
        require_equal(0, subpass_info.pResolveAttachments)
        require_equal(0, subpass_info.pDepthStencilAttachment)
        require_equal(0, subpass_info.preserveAttachmentCount)
        require_equal(0, subpass_info.pPreserveAttachments)

        require_not_equal(0, destroy_render_pass.int_renderPass)
        require_not_equal(0, destroy_render_pass.int_device)


@gapit_test("vkCreateRenderPass_test")
class SingleAttachment(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        create_render_pass = require(self.nth_call_of("vkCreateRenderPass", 2))
        destroy_render_pass = require(self.next_call_of("vkDestroyRenderPass"))

        render_pass_create_info = VulkanStruct(
            architecture, RENDER_PASS_CREATE_INFO,
            get_read_offset_function(create_render_pass,
                                     create_render_pass.hex_pCreateInfo))
        subpass_info = VulkanStruct(architecture, SUBPASS_DESCRIPTION,
                                    get_read_offset_function(
                                        create_render_pass,
                                        render_pass_create_info.pSubpasses))

        attachment_description = VulkanStruct(
            architecture, ATTACHMENT_DESCRIPTION,
            get_read_offset_function(create_render_pass,
                                     render_pass_create_info.pAttachments))

        attachment_reference = VulkanStruct(architecture, ATTACHMENT_REFERENCE,
                                            get_read_offset_function(
                                                create_render_pass,
                                                subpass_info.pColorAttachments))

        require_equal(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                      render_pass_create_info.sType)
        require_equal(0, render_pass_create_info.pNext)
        require_equal(0, render_pass_create_info.flags)
        require_equal(1, render_pass_create_info.attachmentCount)
        require_not_equal(0, render_pass_create_info.pAttachments)
        require_equal(1, render_pass_create_info.subpassCount)
        require_not_equal(0, render_pass_create_info.pSubpasses)
        require_equal(0, render_pass_create_info.dependencyCount)
        require_equal(0, render_pass_create_info.pDependencies)

        require_equal(0, subpass_info.flags)
        require_equal(VK_PIPELINE_BIND_POINT_GRAPHICS,
                      subpass_info.pipelineBindPoint)
        require_equal(0, subpass_info.inputAttachmentCount)
        require_equal(0, subpass_info.pInputAttachments)
        require_equal(1, subpass_info.colorAttachmentCount)
        require_not_equal(0, subpass_info.pColorAttachments)
        require_equal(0, subpass_info.pResolveAttachments)
        require_equal(0, subpass_info.pDepthStencilAttachment)
        require_equal(0, subpass_info.preserveAttachmentCount)
        require_equal(0, subpass_info.pPreserveAttachments)

        require_equal(0, attachment_description.flags)
        require_equal(VK_FORMAT_R8G8B8A8_UNORM, attachment_description.format)
        require_equal(VK_SAMPLE_COUNT_1_BIT, attachment_description.samples)
        require_equal(VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                      attachment_description.loadOp)
        require_equal(VK_ATTACHMENT_STORE_OP_STORE,
                      attachment_description.storeOp)
        require_equal(VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                      attachment_description.stencilLoadOp)
        require_equal(VK_ATTACHMENT_STORE_OP_DONT_CARE,
                      attachment_description.stencilStoreOp)
        require_equal(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                      attachment_description.initialLayout)
        require_equal(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                      attachment_description.finalLayout)

        require_equal(0, attachment_reference.attachment)
        require_equal(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                      attachment_reference.layout)

        require_not_equal(0, destroy_render_pass.int_renderPass)
        require_not_equal(0, destroy_render_pass.int_device)

        if self.not_device(device_properties, 0x5BCE4000, NVIDIA_K2200):
            destroy_render_pass = require(
                self.next_call_of("vkDestroyRenderPass"))
            require_equal(0, destroy_render_pass.int_renderPass)
            require_not_equal(0, destroy_render_pass.int_device)
