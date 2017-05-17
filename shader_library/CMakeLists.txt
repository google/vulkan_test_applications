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
#

add_shader_library(shader_library
  SOURCES
    double_numbers.comp
    hardcode_pos_triangle.frag
    hardcode_pos_triangle.vert
    simple_fragment.frag
    simple_vertex.vert
    test.vert
    foo/test.frag
    foo/test.glsl
    models/model_setup.glsl
)
