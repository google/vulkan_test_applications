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

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec2 texcoord;

layout(set = 0, binding = 2) uniform samplerBuffer alphaData;

float handle_overflow(float f) {
  if (f > 1.0) {
    f = 2.0 - f;
  }
  return f;
}

void main() {
    vec4 c = texelFetch(alphaData, 0);
    c.r = handle_overflow(c.r);
    c.g = handle_overflow(c.g);
    c.b = handle_overflow(c.b);
    c.a = handle_overflow(c.a);
    out_color = c * vec4(texcoord, 1.0, 1.0);
}
