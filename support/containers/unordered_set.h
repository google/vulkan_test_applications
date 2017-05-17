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

#ifndef SUPPORT_CONTAINERS_UNORDERED_SET_H_
#define SUPPORT_CONTAINERS_UNORDERED_SET_H_

#include <unordered_set>

#include "support/containers/stl_compatible_allocator.h"

namespace containers {

// This class specializes unordered_set for the use with our Allocator.
// The methods provided are just there to simplify the construction/destruction
// of the object.
template <typename T, typename Hash = std::hash<T>,
          typename KeyEqual = std::equal_to<T>>
class unordered_set
    : public std::unordered_set<T, Hash, KeyEqual, StlCompatibleAllocator<T>> {
 private:
  using underlying_type =
      std::unordered_set<T, Hash, KeyEqual, StlCompatibleAllocator<T>>;

 public:
  unordered_set(const StlCompatibleAllocator<T>& alloc)
      : underlying_type(0, Hash(), KeyEqual(), alloc) {}
  unordered_set(const unordered_set& other) : underlying_type(other) {}
  unordered_set(unordered_set&& other) : underlying_type(std::move(other)) {}
};

}  // namespace containers

#endif  // SUPPORT_CONTAINERS_UNORDERED_SET_H_
