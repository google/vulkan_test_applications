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


# Everything else depends on sample_application
add_vulkan_subdirectory(sample_application_framework)

set(ALL_SAMPLE_NAMES)
add_custom_target(ALL_SAMPLES)

macro(add_vulkan_sample_application name)
    add_vulkan_executable(${name} ${ARGN})
    add_dependencies(ALL_SAMPLES ${name})
    set(T ${ALL_SAMPLE_NAMES})
    list(APPEND T ${name})
    set(ALL_SAMPLE_NAMES ${T} PARENT_SCOPE)
endmacro()

add_vulkan_subdirectory(dummy)

add_vulkan_subdirectory(async_compute)
add_vulkan_subdirectory(atomic_int64)
add_vulkan_subdirectory(blend_constants)
add_vulkan_subdirectory(blit_image)
add_vulkan_subdirectory(bufferview)
add_vulkan_subdirectory(clear_attachments)
add_vulkan_subdirectory(clear_colorimage)
add_vulkan_subdirectory(clear_depthimage)
add_vulkan_subdirectory(compute_particles)
add_vulkan_subdirectory(conservative_rasterization)
add_vulkan_subdirectory(copy_image_buffer)
add_vulkan_subdirectory(copy_image_2d3d)
add_vulkan_subdirectory(copy_querypool_results)
add_vulkan_subdirectory(copy_querypool_results_host_reset)
add_vulkan_subdirectory(cube)
add_vulkan_subdirectory(debug_utils)
add_vulkan_subdirectory(decorate_string)
add_vulkan_subdirectory(descriptor_update_template)
add_vulkan_subdirectory(depth_bounds)
add_vulkan_subdirectory(depth_range_unrestricted)
add_vulkan_subdirectory(depth_readback)
add_vulkan_subdirectory(dispatch)
add_vulkan_subdirectory(dispatch_indirect)
add_vulkan_subdirectory(draw_indexed_indirect_count)
add_vulkan_subdirectory(driver_properties)
add_vulkan_subdirectory(execute_commands)
add_vulkan_subdirectory(external_buffer)
add_vulkan_subdirectory(external_image)
add_vulkan_subdirectory(fence_test)
add_vulkan_subdirectory(fill_buffer)
add_vulkan_subdirectory(khr_image_format_list)
add_vulkan_subdirectory(many_commandbuffers_cube)
add_vulkan_subdirectory(memory_model)
add_vulkan_subdirectory(mixed_sample_count)
add_vulkan_subdirectory(memory_budget)
add_vulkan_subdirectory(multigpu_particles)
add_vulkan_subdirectory(multiplanar_image_disjoint)
add_vulkan_subdirectory(multiplanar_image_explicit)
add_vulkan_subdirectory(multiplanar_image_non_disjoint)
add_vulkan_subdirectory(mutable_swapchain_format)
add_vulkan_subdirectory(passthrough)
add_vulkan_subdirectory(protected_memory)
add_vulkan_subdirectory(pipeline_creation_feedback)
add_vulkan_subdirectory(pci_bus_info)
add_vulkan_subdirectory(render_3d_image)
add_vulkan_subdirectory(render_input_attachment)
add_vulkan_subdirectory(render_depth_attachment)
add_vulkan_subdirectory(render_quad)
add_vulkan_subdirectory(sampler_mirror_clamp_to_edge)
add_vulkan_subdirectory(shader_core_properties)
add_vulkan_subdirectory(shader_float_controls)
add_vulkan_subdirectory(shader_info)
add_vulkan_subdirectory(shared_presentable_image)
add_vulkan_subdirectory(simple_compute)
add_vulkan_subdirectory(spirv_1_4)
add_vulkan_subdirectory(stencil)
add_vulkan_subdirectory(stencil_export)
add_vulkan_subdirectory(set_event)
add_vulkan_subdirectory(sparse_binding)
add_vulkan_subdirectory(standard_uniform_buffer_layout)
add_vulkan_subdirectory(subgroup_ballot)
add_vulkan_subdirectory(swapchain_colorspace)
add_vulkan_subdirectory(subgroup_vote)
add_vulkan_subdirectory(textured_cube)
add_vulkan_subdirectory(viewport_index)
add_vulkan_subdirectory(wireframe)
add_vulkan_subdirectory(write_timestamp)

if (NOT ANDROID)
    string(REPLACE ";" "\n" ALL_SAMPLE_NAMES "${ALL_SAMPLE_NAMES}")
    write_file(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples.txt ${ALL_SAMPLE_NAMES})
endif()
