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

#include "support/entry/entry.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>

#include "entry_config.h"
#include "support/log/log.h"

namespace entry {
namespace internal {
void dummy_function() {}
}  // namespace internal
}  // namespace entry

#if defined __ANDROID__
#include <android/window.h>
#include <android_native_app_glue.h>
#include <unistd.h>

struct AppData {
  // Poor-man's semaphore, c++11 is missing a semaphore.
  std::mutex start_mutex;
};

// This is a  handler for all android app commands.
// Only handles APP_CMD_INIT_WINDOW right now, which unblocks
// the main thread, which needs the main window to be open
// before it does any WSI integration.
void HandleAppCommand(android_app* app, int32_t cmd) {
  AppData* data = (AppData*)app->userData;
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      if (app->window != NULL) {
        // Wake the thread that is ready to go.
        data->start_mutex.unlock();
      }
      break;
  }
};

// This method is called by android_native_app_glue. This is the main entry
// point for any native android activity.
void android_main(android_app* app) {
  int32_t output_frame = OUTPUT_FRAME;
  const char* output_file = OUTPUT_FILE;

  // Simply wait for 10 seconds, this is useful if we have to attach late.
  if (access("/sdcard/wait-for-debugger.txt", F_OK) != -1) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  ANativeActivity_setWindowFlags(
      app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
  // Hack to make sure android_native_app_glue is not stripped.
  app_dummy();
  AppData data;
  data.start_mutex.lock();

  std::thread main_thread([&]() {
    data.start_mutex.lock();
    int32_t width = output_frame >= 0 ? DEFAULT_WINDOW_WIDTH
                                      : ANativeWindow_getWidth(app->window);
    int32_t height = output_frame >= 0 ? DEFAULT_WINDOW_HEIGHT
                                       : ANativeWindow_getHeight(app->window);

    containers::Allocator root_allocator;
    {
      entry::entry_data data{
          app->window,
          logging::GetLogger(&root_allocator),
          &root_allocator,
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height),
          {FIXED_TIMESTEP, PREFER_SEPARATE_PRESENT, output_file, output_frame}};
      int return_value = main_entry(&data);
      // Do not modify this line, scripts may look for it in the output.
      data.log->LogInfo("RETURN: ", return_value);
    }
    assert(root_allocator.currently_allocated_bytes_.load() == 0);
  });

  app->userData = &data;
  app->onAppCmd = &HandleAppCommand;

  while (1) {
    // Read all pending events.
    int ident = 0;
    int events = 0;
    struct android_poll_source* source = nullptr;
    while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {
      if (source) {
        source->process(app, source);
      }
      if (app->destroyRequested) {
        break;
      }
    }
    if (app->destroyRequested) {
      break;
    }
  }

  main_thread.join();
}
#elif defined __linux__
#include <unistd.h>
static char file_path[1024 * 1024] = {};
// This creates an XCB connection and window for the application.
// It maps it onto the screen and passes it on to the main_entry function.
// -w=X will set the window width to X
// -h=Y will set the window height to Y
int main(int argc, char** argv) {
  int path_len = readlink("/proc/self/exe", file_path, 1024 * 1024 - 1);
  if (path_len != -1) {
    file_path[path_len] = '\0';
    for (size_t i = path_len - 1; i >= 0; --i) {
      // Cut off the exe name
      if (file_path[i] == '/') {
        file_path[i] = '\0';
        break;
      }
    }
    const char* env = getenv("VK_LAYER_PATH");
    std::string layer_path = env ? env : "";
    if (!layer_path.empty()) {
      layer_path = layer_path + ":" + file_path;
    } else {
      layer_path = file_path;
    }
    setenv("VK_LAYER_PATH", layer_path.c_str(), 1);
  }

  uint32_t window_width = DEFAULT_WINDOW_WIDTH;
  uint32_t window_height = DEFAULT_WINDOW_HEIGHT;
  bool fixed_timestep = FIXED_TIMESTEP;
  bool prefer_separate_present = PREFER_SEPARATE_PRESENT;
  int32_t output_frame = OUTPUT_FRAME;
  const char* output_file = OUTPUT_FILE;

  for (size_t i = 0; i < argc; ++i) {
    if (strncmp(argv[i], "-w=", 3) == 0) {
      window_width = atoi(argv[i] + 3);
    }
    if (strncmp(argv[i], "-h=", 3) == 0) {
      window_height = atoi(argv[i] + 3);
    }
    if (strncmp(argv[i], "-fixed", 6) == 0) {
      fixed_timestep = true;
    }
    if (strncmp(argv[i], "-separate-present", 17) == 0) {
      prefer_separate_present = true;
    }
    if (strncmp(argv[i], "-output-frame=", 14) == 0) {
      output_frame = atoi(argv[i] + 14);
    }
    if (strncmp(argv[i], "-output-file=", 13) == 0) {
      output_file = argv[i] + 13;
    }
  }

#if HAVE_CALLBACK_SWAPCHAIN == 1
  if (output_frame != -1) {
    std::cerr << "Callback swapchain was not build -output-frame is not enabled"
              << std::endl;
  }
#endif

  containers::Allocator root_allocator;
  xcb_connection_t* connection;
  xcb_window_t window;
  if (output_frame == -1) {
    connection = xcb_connect(NULL, NULL);
    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    window = xcb_generate_id(connection);
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0,
                      0, window_width, window_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0,
                      NULL);

    xcb_map_window(connection, window);
    xcb_flush(connection);
  }

  int return_value = 0;

  std::thread main_thread([&]() {
    entry::entry_data data{
        window,
        connection,
        logging::GetLogger(&root_allocator),
        &root_allocator,
        window_width,
        window_height,
        {fixed_timestep, prefer_separate_present, output_file, output_frame}};
    return_value = main_entry(&data);
  });
  main_thread.join();
  // TODO(awoloszyn): Handle other events here.
  xcb_disconnect(connection);
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return return_value;
}
#elif defined _WIN32
#error TODO(awoloszyn): Handle the win32 entry point.
#error This means setting up a Win32 window, and passing it through the entry_data.
#else
#error Unsupported platform.
#endif
