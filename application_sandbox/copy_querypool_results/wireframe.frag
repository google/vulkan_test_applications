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
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 out_color;
layout(location = 2) in vec4 normal;

layout(set = 0, binding = 2) uniform isamplerBuffer query_data;

void main() {
    int q = texelFetch(query_data, 0).r % 40000;
    float qr = q;
    if (q != 0.0) {
      int nq = 40000 - q;
      qr = (nq < q ? nq : q) / 20000.0;
      qr = qr < 0.3 ? 0.3 : qr;
    }
    out_color = vec4(vec3(qr), 1.0);
}
