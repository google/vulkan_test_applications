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
add_subdirectory(sample_application_framework)

set(ALL_SAMPLE_NAMES)
add_custom_target(ALL_SAMPLES)

macro(add_vulkan_sample_application name)
    add_vulkan_executable(${name} ${ARGN})
    add_dependencies(ALL_SAMPLES ${name})
    set(T ${ALL_SAMPLE_NAMES})
    list(APPEND T ${name})
    set(ALL_SAMPLE_NAMES ${T} PARENT_SCOPE)
endmacro()

add_subdirectory(async_compute)
add_subdirectory(blend_constants)
add_subdirectory(blit_image)
add_subdirectory(bufferview)
add_subdirectory(clear_attachments)
add_subdirectory(clear_colorimage)
add_subdirectory(clear_depthimage)
add_subdirectory(copy_image_buffer)
add_subdirectory(copy_querypool_results)
add_subdirectory(cube)
add_subdirectory(depth_bounds)
add_subdirectory(depth_readback)
add_subdirectory(dispatch)
add_subdirectory(dispatch_indirect)
add_subdirectory(execute_commands)
add_subdirectory(fill_buffer)
add_subdirectory(stencil)
add_subdirectory(set_event)
add_subdirectory(textured_cube)
add_subdirectory(wireframe)
add_subdirectory(write_timestamp)

if (NOT ANDROID)
    string(REPLACE ";" "\n" ALL_SAMPLE_NAMES "${ALL_SAMPLE_NAMES}")
    write_file(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples.txt ${ALL_SAMPLE_NAMES})
endif()
