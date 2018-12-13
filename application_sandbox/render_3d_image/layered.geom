/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3 * 4) out;

layout (location = 1) in vec2 texcoords[];
layout (location = 1) out vec2 texcoord;

void main(void) {
  for (int face = 0; face < 4; face++) {
    gl_Layer = face;
    for (int i = 0; i<gl_in.length(); i++) {
      gl_Position = gl_in[i].gl_Position;
      texcoord = texcoords[i];
      EmitVertex();
    }
    EndPrimitive();
  }
}
