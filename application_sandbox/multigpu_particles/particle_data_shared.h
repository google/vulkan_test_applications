// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _PARTICLE_DATA_SHARED_H_
#define _PARTICLE_DATA_SHARED_H_

#include "include/math_common.h"

struct draw_data {
  Vector4 position_speed;
};

struct simulation_data {
  Vector4 position_velocity;
};

#define TOTAL_PARTICLES (1024 * 256)
#define COMPUTE_SHADER_LOCAL_SIZE 128

#define TOTAL_MASS (1024.0f * 1024.0f * 8.0f)

#endif  // _PARTICLE_DATA_SHARED_H_