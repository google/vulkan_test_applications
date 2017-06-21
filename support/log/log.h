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

#ifndef SUPPORT_LOG_LOG_H_
#define SUPPORT_LOG_LOG_H_

#include <memory>
#include <sstream>
#include <string>

#include "support/containers/unique_ptr.h"

namespace logging {

// Tests the result of "res op exp" and if the result is not "true"
// then logs an error to LogError of the given log.
#define LOG_EXPECT(op, log, res, exp)                                          \
  do {                                                                         \
    auto x = exp;                                                              \
    auto r = res;                                                              \
    if (!(r op x)) {                                                           \
      (log)->LogError(__FILE__, ":", __LINE__,                                 \
                      "\n  Expected " #res " " #op " " #exp "\n  but got ", r, \
                      " " #op " ", x);                                         \
    }                                                                          \
  } while (0);

// The same as LOG_EXPECT but triggers a crash if it did not succeed.
#define LOG_ASSERT(op, log, res, exp)                                          \
  do {                                                                         \
    auto x = exp;                                                              \
    auto r = res;                                                              \
    if (!(r op x)) {                                                           \
      (log)->LogError(__FILE__, ":", __LINE__,                                 \
                      "\n  Expected " #res " " #op " " #exp "\n  but got ", r, \
                      " " #op " ", x);                                         \
      *reinterpret_cast<volatile int*>(size_t(0)) = 4;                         \
    }                                                                          \
  } while (0);

// Logs a message and then forces the program to crash.
#define LOG_CRASH(log, message)                        \
  do {                                                 \
    (log)->LogError(__FILE__, ":", __LINE__, message); \
    *reinterpret_cast<volatile int*>(intptr_t(0)) = 4; \
  } while (0);

// Logging class base. It provides the functionality to
// generate log messages for use by any inherited classes.
// Ideally this would take an allocator and do all memory
// allocations (in stringstream) using that. Unfortunately
// the c++11 spec is missing allocator-aware streams. :(
// We will have to assume that the STL is doing the right thing here.
class Logger {
 public:
   virtual ~Logger() {}

  // Logs a set of values to the error stream of the logger.
  template <typename... Args>
  void LogError(Args... args) {
    std::ostringstream str;
    LogHelper(&str, args...);
    str << "\n";
    LogErrorString(str.str().c_str());
  }

  // Logs a set of values to the info stream of the logger.
  template <typename... Args>
  void LogInfo(Args... args) {
    std::ostringstream str;
    LogHelper(&str, args...);
    str << "\n";
    LogInfoString(str.str().c_str());
  }

 private:
  // Helper function, the recursive base of LogHelper.
  template <typename T>
  void LogHelper(std::ostringstream* stream, const T& val) {
    *stream << val;
  }

  // Helper function to recursively add elements from Args to
  // the stream.
  template <typename T, typename... Args>
  void LogHelper(std::ostringstream* stream, const T& val, Args... args) {
    *stream << val;
    LogHelper(stream, args...);
  }

  // This should be overriden by child classes to log the
  // input null-terminated
  // string to the STDERR equivalent.
  virtual void LogErrorString(const char* str) = 0;
  // This should be overriden by child classes to log the
  // input null-terminated
  // string to the STDOUT equivalent.
  virtual void LogInfoString(const char* str) = 0;
};

// Returns a platform-specific logger.
containers::unique_ptr<Logger> GetLogger(containers::Allocator* allocator);
}

#endif  // SUPPORT_LOG_LOG_H_
