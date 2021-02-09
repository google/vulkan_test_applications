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

layout(input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput stencil;

void main() {
    vec4 f = subpassLoad(stencil).rgba;
    out_color = vec4(0, 0, 0, 0);
    if (f.r > 0.5) {
	out_color.r = out_color.r + 0.5; 
	}
    if (f.g > 0.5) {
	out_color.r = out_color.r + 0.5; 
	}
    if (f.b > 0.5) {
	out_color.b = out_color.b + 0.5; 
	}
    if (f.a > 0.5) {
	out_color.b = out_color.b + 0.5; 
	}
    out_color.g = 1;
    //out_color = vec4(f);//pow(f.rgb, vec3(10.0, 10.0, 10.0)) , 1.0);
    //out_color = vec4(1, 1, 1, 1);
}
