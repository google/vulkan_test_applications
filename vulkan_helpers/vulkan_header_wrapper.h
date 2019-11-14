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

#ifndef VULKAN_HEADER_WRAPPER_H_
#define VULKAN_HEADER_WRAPPER_H_

#if defined __ANDROID__
#define VK_USE_PLATFORM_ANDROID_KHR 1
#elif defined __linux__
#define VK_USE_PLATFORM_XCB_KHR 1
#elif defined _WIN32
#define VK_USE_PLATFORM_WIN32_KHR 1
#elif defined __APPLE__
#define VK_USE_PLATFORM_MACOS_MVK 1
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#undef VK_NO_PROTOTYPES

#endif  // VULKAN_HEADER_WRAPPER_H_
