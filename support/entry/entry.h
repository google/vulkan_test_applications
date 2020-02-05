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
#elif defined __ggp__
#define VK_USE_PLATFORM_GGP 1
#include <ggp/ggp.h>
#elif defined __linux__
#include <xcb/xcb.h>
#elif defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined __APPLE__
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

// EntryData contains the information about the window and application options
// like fixed time step etc. On Windows and Linux it is used to create a
// window and cache the handles of the window for display.
class EntryData {
 public:
  EntryData(const EntryData&) = delete;
  EntryData(EntryData&&) = delete;
  EntryData& operator=(const EntryData&) = delete;
  EntryData& operator=(EntryData&&) = delete;

  // For Android, the width and height should be the window size provided by
  // the app pointer. For Linux and Windows, the window size should come from
  // command line or the default value.
  EntryData(containers::Allocator* allocator, uint32_t width, uint32_t height,
            bool fixed_timestep, bool separate_present,
            int64_t output_frame_index, const char* output_frame_file,
            const char* shader_compiler
#if defined __ANDROID__
            ,
            android_app* app
#endif
  );

  // dtor of EntryData
  ~EntryData() {
#if defined __ANDROID__
#elif defined _WIN32
    if (native_window_handle_) {
      DestroyWindow(native_window_handle_);
    }
#elif defined __ggp__
// Empty
#elif defined __linux__
    free(delete_window_atom_);
    xcb_disconnect(native_connection_);
#endif
  }

#if defined __ANDROID__
  // Window is not created by the application on Android
#elif defined __linux__
  bool CreateWindow();
#elif defined _WIN32
  bool CreateWindowWin32();
#elif defined __APPLE__
  bool CreateWindow();
#endif
  bool WindowClosing() const;

  void NotifyReady() const;

#if defined __ANDROID__
  ANativeWindow* native_window_handle() const { return native_window_handle_; }
  const char* os_version() const { return os_version_.c_str(); }
  void CloseWindow() { window_closing_ = true; }
#elif defined _WIN32
  HWND native_window_handle() const { return native_window_handle_; }
  HINSTANCE native_hinstance() const { return native_hinstance_; }
#elif defined __ggp__
// Empty
#elif defined __linux__
  xcb_window_t native_window_handle() const { return native_window_handle_; }
  xcb_connection_t* native_connection() const { return native_connection_; }
#elif defined __APPLE__
  void* native_window_handle() const { return native_window_handle_; }
#endif

  logging::Logger* logger() const { return log_.get(); }
  containers::Allocator* allocator() const { return allocator_; }
  bool fixed_timestep() const { return fixed_timestep_; }
  bool prefer_separate_present() const { return prefer_separate_present_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }
  int64_t output_frame_index() const { return output_frame_index_; }
  const char* output_frame_file() const { return output_frame_file_; }
  const char* shader_compiler() const { return shader_compiler_; }

 private:
  bool fixed_timestep_;
  bool prefer_separate_present_;
  uint32_t width_;
  uint32_t height_;
  int64_t output_frame_index_;
  const char* output_frame_file_;
  const char* shader_compiler_;
  containers::unique_ptr<logging::Logger> log_;
  containers::Allocator* allocator_;

#if defined __ANDROID__
  ANativeWindow* native_window_handle_;
  std::string os_version_;
  bool window_closing_;
#elif defined _WIN32
  HINSTANCE native_hinstance_;
  HWND native_window_handle_;
#elif defined __ggp__
// Empty
#elif defined __linux__
  xcb_window_t native_window_handle_;
  xcb_connection_t* native_connection_;
  xcb_intern_atom_reply_t* delete_window_atom_;
#elif defined __APPLE__
  void* native_window_handle_;
#endif
};
}  // namespace entry
// This is the entry-point that every application should define.
int main_entry(const entry::EntryData* data);

#endif  // SUPPORT_ENTRY_ENTRY_H_
