# Copyright 2020 Google Inc.
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

add_shader_library(separate_depth_stencil_layouts_shaders
  SOURCES
    cube.frag
    cube.vert
    torus.frag
    torus.vert
  SHADER_DEPS
    shader_library
)

add_vulkan_sample_application(separate_depth_stencil_layouts
  SOURCES main.cpp
  LIBS
    vulkan_helpers
  MODELS
    standard_models
  SHADERS
    separate_depth_stencil_layouts_shaders
)
