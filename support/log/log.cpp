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

#include "support/log/log.h"
#include <cstring>

namespace logging {
#if defined __ANDROID__
#include <android/log.h>

class InternalLogger : public Logger {
 public:
  void LogErrorString(const char* str) override {
    __android_log_print(ANDROID_LOG_ERROR, "VulkanTestApplication", "%s", str);
  }
  void LogInfoString(const char* str) override {
    __android_log_print(ANDROID_LOG_INFO, "VulkanTestApplication", "%s", str);
  }
};
#elif defined __linux__
#include <cstdio>
class InternalLogger : public Logger {
 public:
  void LogErrorString(const char* str) override {
    fprintf(stderr, "error: ");
    int len = strlen(str);
    int pos = 0;
    while (len) {
      int written = fprintf(stderr, "%s", str + pos);
      pos += written;
      len -= written;
    }
  }

  void LogInfoString(const char* str) override {
    int len = strlen(str);
    int pos = 0;
    while (len) {
      int written = fprintf(stdout, "%s", str + pos);
      pos += written;
      len -= written;
    }
  }

  void Flush() override {
    fflush(stderr);
    fflush(stdout);
  }
};
#elif defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
class InternalLogger : public Logger {
 public:
  InternalLogger() { console_handle_ = GetStdHandle(STD_OUTPUT_HANDLE); }
  void LogErrorString(const char* str) override {
    SetConsoleTextAttribute(console_handle_,
                            FOREGROUND_RED | FOREGROUND_INTENSITY);
    DWORD written = 0;
    WriteConsole(console_handle_,
                 "error: ", static_cast<DWORD>(strlen("error: ")), &written,
                 nullptr);
    SetConsoleTextAttribute(
        console_handle_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    WriteConsole(console_handle_, str, static_cast<DWORD>(strlen(str)),
                 &written, nullptr);
  }
  void LogInfoString(const char* str) override {
    DWORD written = 0;
    WriteConsole(console_handle_, str, static_cast<DWORD>(strlen(str)),
                 &written, nullptr);
  }

 private:
  HANDLE console_handle_;
};
#endif

containers::unique_ptr<Logger> GetLogger(containers::Allocator* allocator) {
  return containers::make_unique<InternalLogger>(allocator);
}
}  // namespace logging