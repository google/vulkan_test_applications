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

#if defined __linux__ || defined _WIN32
struct CommandLineArgs {
  uint32_t window_width;
  uint32_t window_height;
  bool fixed_timestep;
  bool prefer_separate_present;
  int32_t output_frame;
  const char* output_file;
};

void parse_args(CommandLineArgs* args, int argc, const char** argv) {
  args->window_width = DEFAULT_WINDOW_WIDTH;
  args->window_height = DEFAULT_WINDOW_HEIGHT;
  args->fixed_timestep = FIXED_TIMESTEP;
  args->prefer_separate_present = PREFER_SEPARATE_PRESENT;
  args->output_frame = OUTPUT_FRAME;
  args->output_file = OUTPUT_FILE;

  for (int i = 0; i < argc; ++i) {
    if (strncmp(argv[i], "-w=", 3) == 0) {
      args->window_width = atoi(argv[i] + 3);
    }
    if (strncmp(argv[i], "-h=", 3) == 0) {
      args->window_height = atoi(argv[i] + 3);
    }
    if (strncmp(argv[i], "-fixed", 6) == 0) {
      args->fixed_timestep = true;
    }
    if (strncmp(argv[i], "-separate-present", 17) == 0) {
      args->prefer_separate_present = true;
    }
    if (strncmp(argv[i], "-output-frame=", 14) == 0) {
      args->output_frame = atoi(argv[i] + 14);
    }
    if (strncmp(argv[i], "-output-file=", 13) == 0) {
      args->output_file = argv[i] + 13;
    }
  }
}
#endif

#if defined __ANDROID__
#include <android/window.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
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

  // Get the os version
  char os_version_c_str[PROP_VALUE_MAX];
  int os_version_length =
      __system_property_get("ro.build.version.release", os_version_c_str);

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

    containers::LeakCheckAllocator root_allocator;
    {
      entry::entry_data data{
          app->window,
          os_version_length != 0 ? os_version_c_str : "",
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
int main(int argc, const char** argv) {
  int path_len = readlink("/proc/self/exe", file_path, 1024 * 1024 - 1);
  if (path_len != -1) {
    file_path[path_len] = '\0';
    for (ssize_t i = path_len - 1; i >= 0; --i) {
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
  CommandLineArgs args;
  parse_args(&args, argc, argv);

  containers::LeakCheckAllocator root_allocator;
  xcb_connection_t* connection;
  xcb_window_t window;
  if (args.output_frame == -1) {
    connection = xcb_connect(NULL, NULL);
    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    window = xcb_generate_id(connection);
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0,
                      0, args.window_width, args.window_height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0,
                      NULL);

    xcb_map_window(connection, window);
    xcb_flush(connection);
  }

  int return_value = 0;

  std::thread main_thread([&]() {
    entry::entry_data data{window,
                           connection,
                           logging::GetLogger(&root_allocator),
                           &root_allocator,
                           args.window_width,
                           args.window_height,
                           {args.fixed_timestep, args.prefer_separate_present,
                            args.output_file, args.output_frame}};
    return_value = main_entry(&data);
  });
  main_thread.join();
  // TODO(awoloszyn): Handle other events here.
  xcb_disconnect(connection);
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return return_value;
}
#elif defined _WIN32

void write_error(HANDLE handle, const char* message) {
  DWORD written = 0;
  WriteConsole(handle, message, static_cast<DWORD>(strlen(message)), &written,
               nullptr);
}

static char file_path[1024 * 1024] = {};
static char layer_path[1024 * 1024] = {};
int main(int argc, const char** argv) {
  CommandLineArgs args;
  parse_args(&args, argc, argv);

  containers::LeakCheckAllocator root_allocator;
  HINSTANCE instance = 0;
  HWND window_handle = 0;
  HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (out_handle == INVALID_HANDLE_VALUE) {
    AllocConsole();
    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out_handle == INVALID_HANDLE_VALUE) {
      return -1;
    }
  }

  DWORD path_len = GetModuleFileNameA(NULL, file_path, 1024 * 1024 - 1);
  if (path_len != -1) {
    file_path[path_len] = '\0';
    for (DWORD i = path_len - 1; i >= 0; --i) {
      // Cut off the exe name
      if (file_path[i] == '/' || file_path[i] == '\\') {
        file_path[i] = '\0';
        break;
      }
    }
    DWORD d =
        GetEnvironmentVariableA("VK_LAYER_PATH", layer_path, 1024 * 1024 - 1);
    std::string lp = layer_path;
    if (d > 0 && !lp.empty()) {
      lp = lp + ":" + file_path;
    } else {
      lp = file_path;
    }
    SetEnvironmentVariableA("VK_LAYER_PATH", lp.c_str());
  }

  if (args.output_frame == -1) {
    WNDCLASSEX window_class;
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &DefWindowProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.hIcon = NULL;
    window_class.hCursor = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = "Sample application";
    window_class.hIconSm = NULL;
    if (!RegisterClassEx(&window_class)) {
      DWORD num_written = 0;
      write_error(out_handle, "Could not register class");
      return -1;
    }

    RECT rect = {0, 0, LONG(args.window_width), LONG(args.window_height)};

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    window_handle = CreateWindowEx(
        0, "Sample application", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0,
        GetModuleHandle(NULL), NULL);

    if (!window_handle) {
      write_error(out_handle, "Could not create window");
    }

    instance = reinterpret_cast<HINSTANCE>(
        GetWindowLongPtr(window_handle, GWLP_HINSTANCE));
    ShowWindow(window_handle, SW_SHOW);
  }

  int return_value = 0;

  std::thread main_thread([&]() {
    entry::entry_data data{instance,
                           window_handle,
                           logging::GetLogger(&root_allocator),
                           &root_allocator,
                           args.window_width,
                           args.window_height,
                           {args.fixed_timestep, args.prefer_separate_present,
                            args.output_file, args.output_frame}};
    return_value = main_entry(&data);
  });

  MSG msg;
  while (window_handle && GetMessage(&msg, window_handle, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  main_thread.join();
  if (window_handle) {
    DestroyWindow(window_handle);
  }
  return return_value;
}
#else
#error Unsupported platform.
#endif
