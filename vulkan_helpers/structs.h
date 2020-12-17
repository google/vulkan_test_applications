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

#ifndef VULKAN_HELPERS_STRUCTS_H_
#define VULKAN_HELPERS_STRUCTS_H_

#include "support/containers/allocator.h"
#include "support/containers/vector.h"
#include "vulkan_helpers/vulkan_header_wrapper.h"

namespace vulkan {
// Returns all valid VkFormat values via a vector allocated from the given
// allocator.
containers::vector<::VkFormat> AllVkFormats(containers::Allocator* allocator);

// Returns all valid VkImageType values via a vector allocated from the given
// allocator.
containers::vector<::VkImageType> AllVkImageTypes(
    containers::Allocator* allocator);

// Returns all valid VkImageTiling values via a vector allocated from the given
// allocator.
containers::vector<::VkImageTiling> AllVkImageTilings(
    containers::Allocator* allocator);

// Returns all valid combinations of VkImageUsageFlagBits values via a vector
// allocated from the given allocator.
containers::vector<::VkImageUsageFlags> AllVkImageUsageFlagCombinations(
    containers::Allocator* allocator);

// Returns all valid combinations of VkImageCreateFlagBits values via a vector
// allocated from the given allocator.
containers::vector<::VkImageCreateFlags> AllVkImageCreateFlagCombinations(
    containers::Allocator* allocator);

// Decomposes the VkSampleCountFlagBits from the given flags and returns them
// via a vector allocated from the given allocator.
containers::vector<::VkSampleCountFlagBits> DecomposeVkSmapleCountFlagBits(
    ::VkSampleCountFlags flags, containers::Allocator* allocator);

// Returns all valid VkCommandBufferLevel values via a vector allocated from the
// given allocator.
containers::vector<::VkCommandBufferLevel> AllVkCommandBufferLevels(
    containers::Allocator* allocator);

// Returns all valid combinations of VkCommandBufferResetFlagBits values via a
// vector allocated from the given allocator.
containers::vector<::VkCommandBufferResetFlags>
AllVkCommandBufferResetFlagCombinations(containers::Allocator* allocator);
}  // namespace vulkan

#endif  //  VULKAN_HELPERS_STRUCTS_H_
