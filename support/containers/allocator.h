/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <assert.h>
#include <atomic>
#include <cstdlib>
#include <map>
#include <utility>

namespace containers {
struct Allocator {
  virtual void* malloc(size_t val) = 0;
  virtual void free(void* val, size_t size) = 0;

  // Constructs one T from this allocator, while passing
  // down args to the constructor. The memory is allocated
  // from this allocator.
  template <typename T, typename... Args>
  T* construct(Args&&... args) {
    // We assume that the maximum natural alignment for anything
    // is 16 bytes. This will handle all SSE types.
    T* t =
        reinterpret_cast<T*>(16 + static_cast<char*>(malloc(sizeof(T) + 16)));
    size_t* s = reinterpret_cast<size_t*>(reinterpret_cast<char*>(t) - 16);
    *s = sizeof(T) + 16;
    return ::new ((void*)t) T(std::forward<Args>(args)...);
  }

  template <typename T>
  void destroy(T* t) {
    size_t* s = reinterpret_cast<size_t*>(reinterpret_cast<char*>(t) - 16);
    t->~T();
    free(s, *s);
  }
};

// When a user allocates/frees memory from this allocator,
// it tracks the total number of allocations made, the total
// number of bytes current in allocation, and the total number
// of bytes that have ever been allocated. When queried
// from the object, the values are not guaranteed be
// in sync with each other, although each may be correct.
// These numbers are particularly useful at the end of
// this allocator's lifetime, where all child objects
// should have already been destroyed.
struct LeakCheckAllocator : public Allocator {
  LeakCheckAllocator() {
    currently_allocated_bytes_.store(0);
    total_allocated_bytes_.store(0);
    total_number_of_allocations_.store(0);
  }

  // Allocates val bytes from this allocator.
  // Tracks one additional usage, and an additional
  // `val` bytes.
  void* malloc(size_t val) override {
    currently_allocated_bytes_ += val;
    total_allocated_bytes_ += val;
    total_number_of_allocations_ += 1;

    return ::malloc(val);
  }

  // Free val from the allocator.
  // Releases the tracking of `bytes` bytes.
  void free(void* val, size_t bytes) override {
    currently_allocated_bytes_ -= bytes;
    ::free(val);
  }

  std::atomic<size_t> currently_allocated_bytes_;
  std::atomic<uint64_t> total_allocated_bytes_;
  std::atomic<uint64_t> total_number_of_allocations_;
};

#define RELEASE_ASSERT(x)                              \
  do {                                                 \
    if (!x) {                                          \
      *reinterpret_cast<volatile int*>(size_t(0)) = 4; \
    }                                                  \
  } while (false)

struct CheckedAllocator : public Allocator {
  CheckedAllocator(Allocator* _alloc) : m_root_allocator(_alloc) {}
  void* malloc(size_t val) override {
    void* ret_val = m_root_allocator->malloc(val);
    m_allocations[ret_val] = val;
    return ret_val;
  }

  void free(void* val, size_t bytes) override {
    RELEASE_ASSERT(m_allocations[val] == bytes);
    m_allocations.erase(val);
    return m_root_allocator->free(val, bytes);
  }

  std::map<void*, size_t> m_allocations;
  Allocator* m_root_allocator;
};

#undef RELEASE_ASSERT
}  // namespace containers

#endif  // SUPPORT_CONTAINERS_ALLOCATOR_H_