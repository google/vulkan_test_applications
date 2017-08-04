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

add_shader_library(tiny_compute_shaders
  SOURCES
    add_numbers.comp
    add_numbers.cl
    add_numbers.spvasm
  SHADER_DEPS
    shader_library
)

add_vulkan_sample_application(tiny_compute_glsl
  SOURCES main.cpp
    inputs.h
  LIBS
    vulkan_helpers
  SHADERS
    tiny_compute_shaders
)
set_target_properties(tiny_compute_glsl PROPERTIES COMPILE_FLAGS "-DUSE_CL=0")

add_vulkan_sample_application(tiny_compute_cl
  SOURCES main.cpp
    inputs.h
  LIBS
    vulkan_helpers
  SHADERS
    tiny_compute_shaders
)
set_target_properties(tiny_compute_cl PROPERTIES COMPILE_FLAGS "-DUSE_CL=1")

add_vulkan_sample_application(tiny_compute_spvasm
  SOURCES main.cpp
    inputs.h
  LIBS
    vulkan_helpers
  SHADERS
    tiny_compute_shaders
)
set_target_properties(tiny_compute_spvasm PROPERTIES COMPILE_FLAGS "-DUSE_SPVASM=1")
