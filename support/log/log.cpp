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
#else
#include <cstdio>
class InternalLogger : public Logger {
 public:
  void LogErrorString(const char* str) override {
    fprintf(stderr, "error: %s", str);
  }
  void LogInfoString(const char* str) override { fprintf(stdout, "%s", str); }
};
#endif

containers::unique_ptr<Logger> GetLogger(containers::Allocator* allocator) {
  return containers::make_unique<InternalLogger>(allocator);
}
}