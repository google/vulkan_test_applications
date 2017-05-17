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

#ifndef VULKAN_HELPERS_KNOWN_DEVICE_INFOS_H_
#define VULKAN_HELPERS_KNOWN_DEVICE_INFOS_H_

#include <cstdint>

namespace vulkan {

struct DeviceInfo {
  uint32_t vendor_id;
  uint32_t device_id;
};

extern const DeviceInfo PixelC;
}

// Does not run the given code if the given device is running with a driver
// before the given version.
#define IF_NOT_DEVICE(log, vulkan_device, DeviceInfo, version)          \
  if (vulkan_device.device_id() == DeviceInfo.device_id &&              \
      vulkan_device.vendor_id() == DeviceInfo.vendor_id &&              \
      vulkan_device.driver_version() <= version) {                      \
    log->LogInfo("--- Skipping code at\n--- ", __FILE__, ":", __LINE__, \
                 " due to known driver issue");                         \
  } else

#endif  //  VULKAN_HELPERS_KNOWN_DEVICE_INFOS_H_
