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

#include "support/entry/entry_config.h"
#include "support/log/log.h"

#if defined __ANDROID__
#include <android/window.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include <unistd.h>
#elif defined __linux__
#include <unistd.h>
#endif

#if defined __ggp__
#include <ggp/ggp.h>
#endif

namespace entry {

namespace internal {
void dummy_function() {}
}  // namespace internal

EntryData::EntryData(containers::Allocator* allocator, uint32_t width,
                     uint32_t height, bool fixed_timestep,
                     bool separate_present, int64_t output_frame_index,
                     const char* output_frame_file, const char* shader_compiler,
                     bool validation,
                     const char* load_pipeline_cache,
                     const char* write_pipeline_cache
#if defined __ANDROID__
                     ,
                     android_app* app
#endif
                     )
    : fixed_timestep_(fixed_timestep),
      prefer_separate_present_(separate_present),
      width_(width),
      height_(height),
      output_frame_index_(output_frame_index),
      output_frame_file_(output_frame_file),
      shader_compiler_(shader_compiler),
      validation_(validation),
      log_(logging::GetLogger(allocator)),
      allocator_(allocator),
      load_pipeline_cache_(load_pipeline_cache? load_pipeline_cache: ""),
      write_pipeline_cache_(write_pipeline_cache? write_pipeline_cache: "")
#if defined __ANDROID__
      ,
      native_window_handle_(app->window),
      os_version_(""),
      window_closing_(false)
#elif defined _WIN32
      ,
      native_hinstance_(0),
      native_window_handle_(0)
#elif defined __ggp__ // Keep this before __linux__
// Nothing here
#elif defined __linux__
      ,
      native_window_handle_(0),
      native_connection_(nullptr),
      delete_window_atom_(nullptr)
#endif
{
#if defined __ANDROID__
  // Get the os version
  char os_version_c_str[PROP_VALUE_MAX];
  int os_version_length =
      __system_property_get("ro.build.version.release", os_version_c_str);
  os_version_ = os_version_length != 0 ? os_version_c_str : "";
#endif
}

#ifdef __ggp__
static std::atomic<bool> k_window_closing(false);
static std::atomic<bool> k_stream_started(false);
#endif

void EntryData::NotifyReady() const {
}

// Returns true when window is to be closed.
bool EntryData::WindowClosing() const {
#if defined __ANDROID__
  return window_closing_;
#elif defined __ggp__
  return k_window_closing;
#elif defined __linux__
  if (native_connection_ && delete_window_atom_) {
    // The memory of 'event' is 'new'ed by xcb call, so we cannot track it
    // in our allocator.
    auto event = std::unique_ptr<xcb_generic_event_t>(
        xcb_poll_for_event(native_connection_));
    while (event) {
      uint8_t event_code = event->response_type & 0x7f;
      if (event_code == XCB_CLIENT_MESSAGE) {
        auto client_msg =
            reinterpret_cast<xcb_client_message_event_t*>(event.get());
        if (client_msg->data.data32[0] == delete_window_atom_->atom) {
          return true;
        }
      }
      event.reset(xcb_poll_for_event(native_connection_));
    }
  }
  return false;
#elif defined _WIN32
  // TODO: implement this for WIN32
  return false;
#elif defined __APPLE__
  return false;
#endif
}
};  // namespace entry

#if defined __linux__ || defined _WIN32 || \
    defined __APPLE__ && !(defined __ANDROID__)
struct CommandLineArgs {
  uint32_t window_width;
  uint32_t window_height;
  bool fixed_timestep;
  bool prefer_separate_present;
  int32_t output_frame;
  const char* output_file;
  const char* shader_compiler;
  bool wait_for_debugger;
  bool validation;
  const char* load_pipeline_cache;
  const char* write_pipeline_cache;
};

void parse_args(CommandLineArgs* args, int argc, const char** argv) {
  args->window_width = DEFAULT_WINDOW_WIDTH;
  args->window_height = DEFAULT_WINDOW_HEIGHT;
  args->fixed_timestep = FIXED_TIMESTEP;
  args->prefer_separate_present = PREFER_SEPARATE_PRESENT;
  args->output_frame = OUTPUT_FRAME;
  args->output_file = OUTPUT_FILE;
  args->shader_compiler = SHADER_COMPILER;
  args->wait_for_debugger = false;
  args->validation = false;
  args->load_pipeline_cache = nullptr;
  args->write_pipeline_cache = nullptr;

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
    if (strncmp(argv[i], "-load-pipeline-cache=", 21) == 0) {
      args->load_pipeline_cache = argv[i] + 21;
    }
    if (strncmp(argv[i], "-write-pipeline-cache=", 22) == 0) {
      args->write_pipeline_cache = argv[i] + 22;
    }
    if (strncmp(argv[i], "-validation", 11) == 0) {
      args->validation = true;
    }
    if (strncmp(argv[i], "-output-file=", 13) == 0) {
      args->output_file = argv[i] + 13;
    }
    if (strncmp(argv[i], "-shader-compiler=", 17) == 0) {
      args->shader_compiler = argv[i] + 17;
    }
    if (strncmp(argv[i], "--wait-for-debugger", 19) == 0) {
      args->wait_for_debugger = true;
    }
  }
}
#endif

#if defined __ANDROID__

struct AppData {
  // Poor-man's semaphore, c++11 is missing a semaphore.
  std::mutex start_mutex;
  entry::EntryData* entry_data;
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
    case APP_CMD_TERM_WINDOW:
      if (data->entry_data != nullptr) {
        data->entry_data->CloseWindow();
      }
      break;
  }
};

// This method is called by android_native_app_glue. This is the main entry
// point for any native android activity.
void android_main(android_app* app) {
  int32_t output_frame = OUTPUT_FRAME;
  const char* output_file = OUTPUT_FILE;
  const char* shader_compiler = SHADER_COMPILER;

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
      entry::EntryData entry_data(&root_allocator, static_cast<uint32_t>(width),
                                  static_cast<uint32_t>(height), FIXED_TIMESTEP,
                                  PREFER_SEPARATE_PRESENT, output_frame,
                                  output_file, shader_compiler, false, nullptr, nullptr, app);
      data.entry_data = &entry_data;
      int return_value = main_entry(&entry_data);
      // Do not modify this line, scripts may look for it in the output.
      entry_data.logger()->LogInfo("RETURN: ", return_value);
      ANativeActivity_finish(app->activity);
    }
    assert(root_allocator.currently_allocated_bytes_.load() == 0);
  });

  app->userData = &data;
  app->onAppCmd = &HandleAppCommand;

  while (true) {
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

#elif defined __ggp__ // Keep this before __linux__
bool entry::EntryData::CreateWindow() { return true; }
static void HandleStreamChanged(const ggp::StreamStateChangedEvent& event) {
  if (event.new_state == ggp::StreamState::kStarted) {
    entry::k_stream_started = true;
  } else if (event.new_state == ggp::StreamState::kExited) {
    entry::k_window_closing = true;
  }
}

uint32_t         stream_state_changed_handler_id   = 0;
static ggp::EventQueue* ggp_event_queue = nullptr;
static char file_path[1024 * 1024] = {};

// Main for Ggp
// This initialized Ggp
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
  ggp::Initialize();

  ggp_event_queue = new ggp::EventQueue();
  assert(ggp_event_queue);
  stream_state_changed_handler_id = ggp::AddStreamStateChangedHandler(
            ggp_event_queue, HandleStreamChanged);

  CommandLineArgs args;
  parse_args(&args, argc, argv);
  while (args.wait_for_debugger)
    ;
  int return_value = 0;
  containers::LeakCheckAllocator root_allocator;
  {
    entry::EntryData entry_data(&root_allocator, args.window_width,
                                args.window_height, args.fixed_timestep,
                                args.prefer_separate_present, args.output_frame,
                                args.output_file, args.shader_compiler, args.validation,
                                args.load_pipeline_cache, args.write_pipeline_cache);
    if (args.output_frame == -1) {
      bool window_created = entry_data.CreateWindow();
      if (!window_created) {
        entry_data.logger()->LogError("Window creation failed");
        return -1;
      }
    }
    std::thread main_thread([&entry_data, &return_value]() {
      while(!entry::k_stream_started) {} // Wait for the stream to be started
      return_value = main_entry(&entry_data);
    });
    std::atomic<bool> exited(false);
    std::thread ggp_thread([&exited]() {
      while(!exited) {
        while(ggp_event_queue->ProcessEvent()) {}
      }
    });
    main_thread.join();
    exited = true;
    ggp_thread.join();

    ggp::RemoveStreamStateChangedHandler(stream_state_changed_handler_id);

    // Indicate that ggp should shutdown.
    ggp::StopStream();
  }
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return return_value;
}
 
#elif defined __linux__

// Create Xcb window
bool entry::EntryData::CreateWindow() {
  if (output_frame_index_ == -1) {
    native_connection_ = xcb_connect(NULL, NULL);

    const xcb_setup_t* setup = xcb_get_setup(native_connection_);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    native_window_handle_ = xcb_generate_id(native_connection_);
    xcb_create_window(native_connection_, XCB_COPY_FROM_PARENT,
                      native_window_handle_, screen->root, 0, 0, width_,
                      height_, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);
    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(native_connection_, 1, 12, "WM_PROTOCOLS");
    auto reply = std::unique_ptr<xcb_intern_atom_reply_t>(
        xcb_intern_atom_reply(native_connection_, cookie, 0));
    xcb_intern_atom_cookie_t cookie2 =
        xcb_intern_atom(native_connection_, 0, 16, "WM_DELETE_WINDOW");
    delete_window_atom_ = xcb_intern_atom_reply(native_connection_, cookie2, 0);
    xcb_change_property(native_connection_, XCB_PROP_MODE_REPLACE,
                        native_window_handle_, reply->atom, 4, 32, 1,
                        &delete_window_atom_->atom);

    xcb_map_window(native_connection_, native_window_handle_);
    xcb_flush(native_connection_);
    return true;
  }
  return false;
}

static char file_path[1024 * 1024] = {};

// Main for Linux
// This creates an XCB connection and window for the application.
// It maps it onto the screen and passes it on to the main_entry function.
// -w=X will set the window width to X
// -h=Y will set the window height to Y
int main(int argc, const char** argv) {
  CommandLineArgs args;
  parse_args(&args, argc, argv);

  if (args.output_frame != -1) {
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
  }
  while (args.wait_for_debugger)
    ;

  int return_value = 0;
  containers::LeakCheckAllocator root_allocator;
  {
    entry::EntryData entry_data(
        &root_allocator, args.window_width, args.window_height,
        args.fixed_timestep, args.prefer_separate_present, args.output_frame,
        args.output_file, args.shader_compiler, args.validation,
        args.load_pipeline_cache, args.write_pipeline_cache);
    if (args.output_frame == -1) {
      bool window_created = entry_data.CreateWindow();
      if (!window_created) {
        entry_data.logger()->LogError("Window creation failed");
        return -1;
      }
    }
    std::thread main_thread([&entry_data, &return_value]() {
      return_value = main_entry(&entry_data);
    });
    main_thread.join();
  }
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return return_value;
}

#elif defined _WIN32

void write_error(HANDLE handle, const char* message) {
  DWORD written = 0;
  WriteConsole(handle, message, static_cast<DWORD>(strlen(message)), &written,
               nullptr);
}

// Create Win32 window
bool entry::EntryData::CreateWindowWin32() {
  HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (out_handle == INVALID_HANDLE_VALUE) {
    AllocConsole();
    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out_handle == INVALID_HANDLE_VALUE) {
      return false;
    }
  }

  if (output_frame_index_ == -1) {
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
      return false;
    }
    RECT rect = {0, 0, LONG(width_), LONG(height_)};

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    native_window_handle_ = CreateWindowEx(
        0, "Sample application", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0,
        GetModuleHandle(NULL), NULL);

    if (!native_window_handle_) {
      write_error(out_handle, "Could not create window");
      return false;
    }

    native_hinstance_ = reinterpret_cast<HINSTANCE>(
        GetWindowLongPtr(native_window_handle_, GWLP_HINSTANCE));
    ShowWindow(native_window_handle_, SW_SHOW);
    return true;
  }
  return false;
}

static char file_path[1024 * 1024] = {};
static char layer_path[1024 * 1024] = {};
// Main for WIN32
int main(int argc, const char** argv) {
  CommandLineArgs args;
  parse_args(&args, argc, argv);

  if (args.output_frame != -1) {
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
        lp = lp + ";" + file_path;
      } else {
        lp = file_path;
      }
      SetEnvironmentVariableA("VK_LAYER_PATH", lp.c_str());
    }
  }
  containers::LeakCheckAllocator root_allocator;
  entry::EntryData entry_data(
      &root_allocator, args.window_width, args.window_height,
      args.fixed_timestep, args.prefer_separate_present, args.output_frame,
      args.output_file, args.shader_compiler, args.validation,
      args.load_pipeline_cache, args.write_pipeline_cache);

  if (args.output_frame == -1) {
    bool window_created = entry_data.CreateWindowWin32();
    if (!window_created) {
      entry_data.logger()->LogError("Window creation failed");
      return -1;
    }
  }
  int return_value = 0;
  std::thread main_thread([&entry_data, &return_value]() {
    return_value = main_entry(&entry_data);
  });

  MSG msg;
  while (entry_data.native_window_handle() &&
         GetMessage(&msg, entry_data.native_window_handle(), 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  main_thread.join();
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return return_value;
}
#endif

#ifdef __APPLE__
extern "C" {
  void RunMacOS();
  void StopMacOS();
  void* CreateMacOSWindow(uint32_t width, uint32_t height);
  bool entry::EntryData::CreateWindow() {
    void* v = CreateMacOSWindow(this->width_, this->height_);
    native_window_handle_ = v;
    return v;
  }

  int main(int argc, const char** argv) {
    CommandLineArgs args;
    parse_args(&args, argc, argv);
    while (args.wait_for_debugger)
      ;
    containers::LeakCheckAllocator root_allocator;
    entry::EntryData entry_data(
        &root_allocator, args.window_width, args.window_height,
        args.fixed_timestep, args.prefer_separate_present, args.output_frame,
        args.output_file, args.shader_compiler, args.validation,
        args.load_pipeline_cache, args.write_pipeline_cache);
    if (args.output_frame == -1) {
      bool window_created = entry_data.CreateWindow();
        if (!window_created) {
          entry_data.logger()->LogError("Window creation failed");
          return -1;
        }
    }
    int ret;
    std::thread run([&ret, &entry_data]() {
      ret = main_entry(&entry_data);
      StopMacOS();
    });
    RunMacOS();
  
  assert(root_allocator.currently_allocated_bytes_.load() == 0);
  return ret;
}
}
#endif
