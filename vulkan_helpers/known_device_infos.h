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

#include <support/log/log.h>

#include <cstdint>
#include <string>

namespace vulkan {

struct DeviceInfo {
  uint32_t vendor_id;
  uint32_t device_id;
};

extern const DeviceInfo PixelC;
extern const DeviceInfo NvidiaK2200;
extern const DeviceInfo Nvidia965M;
}

namespace {
// Version converts a %d.%d.%d string into a Version instance which support
// less-or-equal operator: '<='. Only digits, '.' and blank spaces are allowed
// in the input version string. All other characters will be converted to 0.
struct Version {
  Version(const char* version_string)
      : major_val(0), minor_val(0), revision_val(0) {
    sscanf(version_string, "%d.%d.%d", &major_val, &minor_val, &revision_val);
  }
  bool operator<=(const Version& other) {
    if (major_val > other.major_val) {
      return false;
    }
    if (major_val < other.major_val) {
      return true;
    }
    if (minor_val > other.minor_val) {
      return false;
    }
    if (minor_val < other.minor_val) {
      return true;
    }
    if (revision_val > other.revision_val) {
      return false;
    }
    if (revision_val < other.revision_val) {
      return true;
    }
    return true;
  }

  int major_val, minor_val, revision_val;
};

// Returns true and prints debug info with the given file name and line number
// if the |given_version| is lower or euqal to the |check_version|. Otherwise
// returns false.
bool EqualOrLowerAndroidVersion(logging::Logger* log,
                                const std::string given_version,
                                const std::string check_version,
                                const char* file, int line) {
  if (Version(given_version.c_str()) <= Version(check_version.c_str())) {
    log->LogInfo("--- Skipping code at\n--- ", file, ":", line,
                 " due to known issue with Android version");
    return true;
  }
  return false;
}

// Returns true and prints debug info with the given file name and line number
// if the |given_device_id| is the same as |check_device_id|, the
// |given_vendor_id| is the same as |check_vendor_id| and the
// |given_driver_version| is lower or equal to the |given_driver_version|.
// Otherwise returns false.
bool IsDeviceWithLowerDriverVersion(
    logging::Logger* log, uint32_t given_device_id, uint32_t given_vendor_id,
    uint32_t given_driver_version, uint32_t check_device_id,
    uint32_t check_vendor_id, uint32_t check_driver_version, const char* file,
    int line) {
  log->LogInfo("--- given driver version:", given_driver_version);
  if (given_device_id == check_device_id &&
      given_vendor_id == check_vendor_id &&
      given_driver_version <= check_driver_version) {
    log->LogInfo("--- Skipping code at\n--- ", file, ":", line,
                 " due to known driver issue");
    return true;
  }
  return false;
}
}

// Macro to check if the given |vulkan_device| does NOT have the same device ID
// and vendor ID as the |check_device_info| or a lower/equal driver version
// than |check_driver_version|.
#define NOT_DEVICE(log, vulkan_device, check_device_info,          \
                   check_driver_version)                           \
  (!IsDeviceWithLowerDriverVersion(                                \
      log, vulkan_device.device_id(), vulkan_device.vendor_id(),   \
      vulkan_device.driver_version(), check_device_info.device_id, \
      check_device_info.vendor_id, check_driver_version, __FILE__, __LINE__))

// Macro to check if the Android application is NOT running on the
// |check_version| of Android. 'true' if the application is running on other
// OS.
#ifdef __ANDROID__
#define NOT_ANDROID_VERSION(entry_data, check_os_version)                     \
  (!EqualOrLowerAndroidVersion(entry_data->log.get(), entry_data->os_version, \
                               check_os_version, __FILE__, __LINE__))
#else
#define NOT_ANDROID_VERSION(entry_data, check_os_version) true
#endif

#endif  //  VULKAN_HELPERS_KNOWN_DEVICE_INFOS_H_
