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
#include "include/math_common.h"

layout(location = 0) out vec4 out_color;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in float speed;
layout(set = 0, binding = 1) uniform sampler default_sampler;
layout(set = 0, binding = 2) uniform texture2D default_texture;

// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
Vector3 hsv2rgb(Vector3 hsv) {
    float      hh, p, q, t, ff;
    int        i;
    Vector3         rgb;

    hh = hsv.x;
    while(hh >= 360.0f) hh -= 360.0;
    hh /= 60.0f;
    i = int(hh);
    ff = hh - i;
    p = hsv.z * (1.0 - hsv.y);
    q = hsv.z * (1.0 - (hsv.y * ff));
    t = hsv.z * (1.0 - (hsv.y * (1.0 - ff)));

    switch(i) {
    case 0:
        rgb.r = hsv.z;
        rgb.g = t;
        rgb.b = p;
        break;
    case 1:
        rgb.r = q;
        rgb.g = hsv.z;
        rgb.b = p;
        break;
    case 2:
        rgb.r = p;
        rgb.g = hsv.z;
        rgb.b = t;
        break;

    case 3:
        rgb.r = p;
        rgb.g = q;
        rgb.b = hsv.z;
        break;
    case 4:
        rgb.r = t;
        rgb.g = p;
        rgb.b = hsv.z;
        break;
    case 5:
    default:
        rgb.r = hsv.z;
        rgb.g = p;
        rgb.b = q;
        break;
    }
    return rgb;
}


void main() {
    vec4 color = texture(sampler2D(default_texture, default_sampler), texcoord);
    out_color = vec4(hsv2rgb(Vector3(180.0f + speed * 360.0f, 1.0, 1.0)), color.x * 0.05f);
}