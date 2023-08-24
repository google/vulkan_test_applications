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

#ifndef VULKAN_WRAPPER_LAZY_FUNCTION_H_
#define VULKAN_WRAPPER_LAZY_FUNCTION_H_

// This wraps a lazily initialized function pointer. It will be resolved
// when it is first called.
template <typename T, typename HANDLE, typename WRAPPER>
class LazyFunction {
 public:
  // We retain a reference to the function name, so it must remain valid.
  // In practice this is expected to be used with string constants.
  LazyFunction(HANDLE handle, const char* function_name, WRAPPER* wrapper)
      : handle_(handle), function_name_(function_name), wrapper_(wrapper) {}

  // When this functor is called, it will check if the function pointer
  // has been resolved. If not it will resolve it and then call the function.
  // If it could not be resolved, the program will segfault.
  template <typename... Args>
  typename std::result_of<T(Args...)>::type operator()(const Args&... args);

 private:
  HANDLE handle_;
  const char* function_name_;
  WRAPPER* wrapper_;
  T ptr_ = nullptr;
};

template <typename T, typename HANDLE, typename WRAPPER>
template <typename... Args>
typename std::result_of<T(Args...)>::type
LazyFunction<T, HANDLE, WRAPPER>::operator()(const Args&... args) {
  if (!ptr_) {
    ptr_ = reinterpret_cast<T>(wrapper_->getProcAddr(handle_, function_name_));
    if (ptr_) {
      wrapper_->GetLogger()->LogInfo(function_name_, " for instance ", handle_,
                                     " resolved");
    } else {
      wrapper_->GetLogger()->LogError(function_name_, " for instance ", handle_,
                                      " could not be resolved, crashing now");
    }
  }
  return ptr_(args...);
}

#endif  //  VULKAN_WRAPPER_LAZY_FUNCTION_H_