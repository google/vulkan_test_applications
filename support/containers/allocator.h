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

#ifndef SUPPORT_CONTAINERS_ALLOCATOR_H_
#define SUPPORT_CONTAINERS_ALLOCATOR_H_

#include <atomic>
#include <cstdlib>
#include <utility>

namespace containers {
// When a user allocates/frees memory from this allocator,
// it tracks the total number of allocations made, the total
// number of bytes current in allocation, and the total number
// of bytes that have ever been allocated. When queried
// from the object, the values are not guaranteed be
// in sync with each other, although each may be correct.
// These numbers are particularly useful at the end of
// this allocator's lifetime, where all child objects
// should have already been destroyed.
struct Allocator {
  Allocator() {
    currently_allocated_bytes_.store(0);
    total_allocated_bytes_.store(0);
    total_number_of_allocations_.store(0);
  }

  // Allocates val bytes from this allocator.
  // Tracks one additional usage, and an additional
  // `val` bytes.
  void* malloc(size_t val) {
    currently_allocated_bytes_ += val;
    total_allocated_bytes_ += val;
    total_number_of_allocations_ += 1;

    return ::malloc(val);
  }

  // Free val from the allocator.
  // Releases the tracking of `bytes` bytes.
  void free(void* val, size_t bytes) {
    currently_allocated_bytes_ -= bytes;
    ::free(val);
  }

  // Constructs one T from this allocator, while passing
  // down args to the constructor. The memory is allocated
  // from this allocator.
  template <typename T, typename... Args>
  T* construct(Args&&... args) {
    T* t = static_cast<T*>(malloc(sizeof(T)));
    return ::new ((void*)t) T(std::forward<Args>(args)...);
  }
  std::atomic<size_t> currently_allocated_bytes_;
  std::atomic<uint64_t> total_allocated_bytes_;
  std::atomic<uint64_t> total_number_of_allocations_;
};

}  // namespace containers

#endif  // SUPPORT_CONTAINERS_ALLOCATOR_H_