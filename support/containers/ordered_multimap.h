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

#ifndef SUPPORT_CONTAINERS_ORDERED_MULTI_MAP_H
#define SUPPORT_CONTAINERS_ORDERED_MULTI_MAP_H

#include <map>
#include "support/containers/stl_compatible_allocator.h"

namespace containers {

// This class specializes multimap for the use with our Allocator.
// The methods provided are just there to simplify the construction/destruction
// of the object.
template <typename Key, typename T, typename Compare = std::less<Key>>
class ordered_multimap
    : public std::multimap<Key, T, Compare,
                           StlCompatibleAllocator<std::pair<const Key, T>>> {
 private:
  using underlying_type =
      std::multimap<Key, T, Compare,
                    StlCompatibleAllocator<std::pair<const Key, T>>>;

 public:
  ordered_multimap(const StlCompatibleAllocator<T>& alloc)
      : underlying_type(Compare(), alloc) {}

  ordered_multimap(const ordered_multimap& other) : underlying_type(other) {}
  ordered_multimap(ordered_multimap&& other)
      : underlying_type(std::move(other)) {}
};
}
#endif  // SUPPORT_CONTAINERS_ORDERED_MULTI_MAP_H