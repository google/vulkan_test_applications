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

#ifndef SUPPORT_CONTAINERS_UNIQUE_PTR_H
#define SUPPORT_CONTAINERS_UNIQUE_PTR_H

#include <memory>

#include "support/containers/stl_compatible_allocator.h"
namespace containers {

// Using the unique_deleter that passes around size, lets us cast
// between object types and still correctly track. This would not
// be possible with a lambda or typed struct since we cannot
// easily cast between the deleter types.
class UniqueDeleter {
 public:
  UniqueDeleter() : alloc_(nullptr), original_size_(0) {}
  UniqueDeleter(Allocator* alloc, size_t original_size)
      : alloc_(alloc), original_size_(original_size) {}

  UniqueDeleter(const UniqueDeleter& other)
      : alloc_(other.alloc_), original_size_(other.original_size_) {}
  UniqueDeleter(UniqueDeleter&& other)
      : alloc_(other.alloc_), original_size_(other.original_size_) {}

  UniqueDeleter& operator=(const UniqueDeleter& other) {
    alloc_ = other.alloc_;
    original_size_ = other.original_size_;
    return *this;
  }

  // Since unique_ptr can be cast, the UniqueDeleter will get
  // copied or moved with it. When the time comes to free the
  // object from the unique_ptr, we have to free all
  // of the memory that was originally allocated,
  // which may not be sizeof(T), since T may have been cast to
  // a new type.
  template <typename T>
  void operator()(T* t) {
    if (alloc_) {
      t->~T();
      alloc_->free(static_cast<void*>(t), original_size_);
    }
  }

 private:
  Allocator* alloc_;
  size_t original_size_;
};

template <typename T>
using unique_ptr = std::unique_ptr<T, UniqueDeleter>;

// Makes a new unique_ptr. All memory is allocated from
// the provided allocator, and when the object goes out of scope
// the memory will be freed from it as well.
template <typename T, typename... Args>
unique_ptr<T> make_unique(Allocator* alloc, Args&&... args) {
  T* t = static_cast<T*>(alloc->malloc(sizeof(T)));
  return unique_ptr<T>(::new ((void*)t) T(std::forward<Args>(args)...),
                       UniqueDeleter(alloc, sizeof(T)));
}
}  // namespace containers
#endif  // SUPPORT_CONTAINERS_UNIQUE_PTR_H