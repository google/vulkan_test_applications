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

add_shader_library(transform_feedback_shaders
  SOURCES
    particle_update.vert
    particle.vert
    particle.frag
    particle_data_shared.h
  SHADER_DEPS
    shader_library
    math_common_glsl
)

add_vulkan_sample_application(transform_feedback
  SOURCES main.cpp
  particle_data_shared.h
  LIBS
    vulkan_helpers
    math_common
  MODELS
    standard_models
  SHADERS
    transform_feedback_shaders
  TEXTURES
    standard_images
)
