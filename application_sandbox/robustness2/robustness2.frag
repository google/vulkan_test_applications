/* Copyright 2020 Google Inc.
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
layout (location = 1) in vec2 texcoord;

layout (binding = 2, set = 0) uniform robust_uniform_buffer {
    uint uniform_data[];
};

layout (binding = 3, set = 0) buffer robust_storage_buffer {
    uint storage_data[];
};

layout (rgba8, binding = 4, set = 0) readonly uniform image2D img;

layout (binding = 5, set = 0) buffer null_buffer {
	uint null_data[];
};

void main() {
    out_color = vec4(texcoord, 0.0, 1.0);

    // Storage buffer write out of bounds of the descriptor range.
    if (storage_data[512] == 123) {
        out_color = vec4(0.1, 1.0f, 0.1, 1.0);
    }
    storage_data[512] = 123;

    // Uniform buffer access out of bounds of the descriptor range.
    if (uniform_data[512] == 456) {
        out_color = vec4(0.1, 0.1, 1.0, 1.0);
    }

    // Image load out of bounds.
    out_color = out_color + imageLoad(img, ivec2(256, 256));

    // Null descriptor set.
    null_data[0] = 123456;
}
