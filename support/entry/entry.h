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

#ifndef SUPPORT_ENTRY_ENTRY_H_
#define SUPPORT_ENTRY_ENTRY_H_

#include <functional>
#include <memory>

#include "support/containers/allocator.h"
#include "support/containers/unique_ptr.h"
#include "support/log/log.h"

#if defined __ANDROID__
struct android_app;
struct ANativeWindow;
#elif defined __linux__
#include <xcb/xcb.h>
#elif defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error Unsupported platform
#endif

namespace entry {
#if defined __ANDROID__ || defined __linux__
namespace internal {
// Hack to make sure that this entry-point library gets linked in properly.
void dummy_function();

struct dummy {
  dummy() { dummy_function(); }
};
}  // namespace internal

static internal::dummy __attribute__((used)) test_dummy;
#endif

// If output_frame is > -1, then the given image frame will be written
// to output_file, otherwise the application will render to the screen.
struct application_options {
  bool fixed_timestep;
  bool prefer_separate_present;
  const char* output_file;
  int32_t output_frame;
  const char* shader_compiler;
};

struct entry_data {
#if defined __ANDROID__
  ANativeWindow* native_window_handle;
  std::string os_version;
#elif defined _WIN32
  HINSTANCE native_hinstance;
  HWND native_window_handle;
#elif defined __linux__
  xcb_window_t native_window_handle;
  xcb_connection_t* native_connection;
#endif
  // This is never null.
  containers::unique_ptr<logging::Logger> log;
  containers::Allocator* root_allocator;
  uint32_t width;
  uint32_t height;
  application_options options;
};
}  // namespace entry

// This is the entry-point that every application should define.
int main_entry(const entry::entry_data* data);

#endif  // SUPPORT_ENTRY_ENTRY_H_
