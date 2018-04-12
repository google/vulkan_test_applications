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
from gapit_test_framework import get_read_offset_function
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, UINT64_T, ARRAY, POINTER, FLOAT
from gapit_test_framework import NVIDIA_K2200, NVIDIA_965M


@gapit_test("vkDestroyNullHandles_test")
class AllDestroy(GapitTest):

    def expect(self):
        arch = self.architecture
        device_properties = require(self.next_call_of(
            "vkGetPhysicalDeviceProperties"))

        if (self.not_device(device_properties, 0x5BCE4000, NVIDIA_K2200) and
            self.not_device(device_properties, 0x5BCE4000, NVIDIA_965M)):
                destroy_pipeline_cache=require(
                    self.next_call_of("vkDestroyPipelineCache"))
                require_not_equal(0, destroy_pipeline_cache.int_device)
                require_equal(0, destroy_pipeline_cache.int_pipelineCache)

                free_descriptor_set=require(
                    self.next_call_of("vkFreeDescriptorSets"))
                p_sets=free_descriptor_set.hex_pDescriptorSets
                for i in range(2):
                    actual_set=little_endian_bytes_to_int(
                        require(free_descriptor_set.get_read_data(
                            p_sets + NON_DISPATCHABLE_HANDLE_SIZE * i,
                            NON_DISPATCHABLE_HANDLE_SIZE)))
                    require_equal(0, actual_set)

                destroy_buffer=require(self.next_call_of("vkDestroyBuffer"))
                require_not_equal(0, destroy_buffer.int_device)
                require_equal(0, destroy_buffer.int_buffer)

                destroy_null_buffer_view=require(self.next_call_of(
                    "vkDestroyBufferView"))
                require_not_equal(0, destroy_null_buffer_view.int_device)
                require_equal(0, destroy_null_buffer_view.int_bufferView)

                destroy_descriptor_set_layout=require(self.next_call_of(
                    "vkDestroyDescriptorSetLayout"))
                require_equal(0,
                              destroy_descriptor_set_layout.int_descriptorSetLayout)

                destroy_image=require(self.next_call_of("vkDestroyImage"))
                require_equal(0, destroy_image.int_image)

                destroy_image_view=require(
                    self.next_call_of("vkDestroyImageView"))
                require_equal(0, destroy_image_view.int_imageView)

                destroy_null_query_pool=require(self.next_call_of(
                    "vkDestroyQueryPool"))
                require_not_equal(0, destroy_null_query_pool.int_device)
                require_equal(0, destroy_null_query_pool.int_queryPool)

                destroy_sampler=require(
                    self.next_call_of("vkDestroySampler"))
                require_equal(0, destroy_sampler.int_sampler)

                destroy_descriptor_pool=require(
                    self.next_call_of("vkDestroyDescriptorPool"))
                require_equal(0, destroy_descriptor_pool.int_descriptorPool)

                free_memory=require(self.next_call_of("vkFreeMemory"))
                require_not_equal(0, free_memory.int_device)
                require_equal(0, free_memory.int_memory)

                destroy_pipeline_cache=require(self.next_call_of(
                    "vkDestroyPipelineCache"))
                require_not_equal(0, destroy_pipeline_cache.int_device)
                require_equal(0, destroy_pipeline_cache.int_pipelineCache)

                destroy_semaphore_null=require(
                    self.next_call_of("vkDestroySemaphore"))
                require_equal(0, destroy_semaphore_null.int_semaphore)

                destroy_framebuffer=require(
                    self.next_call_of("vkDestroyFramebuffer"))
                require_equal(0, destroy_framebuffer.int_framebuffer)
                require_not_equal(0, destroy_framebuffer.int_device)
                require_equal(0, destroy_framebuffer.hex_pAllocator)

                destroy_pipeline=require(
                    self.next_call_of("vkDestroyPipelineLayout"))
                require_equal(0, destroy_pipeline.int_pipelineLayout)
                require_not_equal(0, destroy_pipeline.int_device)
                require_equal(0, destroy_pipeline.hex_pAllocator)

                destroy_render_pass=require(
                    self.next_call_of("vkDestroyRenderPass"))
                require_equal(0, destroy_render_pass.int_renderPass)
                require_not_equal(0, destroy_render_pass.int_device)

                destroy_shader_module=require(
                    self.next_call_of("vkDestroyShaderModule"))
                require_not_equal(0, destroy_shader_module.int_device)
                require_equal(0, destroy_shader_module.int_shaderModule)

                destroy_fence=require(self.next_call_of("vkDestroyFence"))
                require_not_equal(0, destroy_fence.int_device)
                require_equal(0, destroy_fence.int_fence)
                require_equal(0, destroy_fence.hex_pAllocator)

                # Things that fail on android 7.1.1
                if self.not_android_version("7.1.1"):
                    destroySwapchain=require(self.next_call_of(
                        "vkDestroySwapchainKHR"))
                    require_equal(0, destroySwapchain.int_swapchain)
