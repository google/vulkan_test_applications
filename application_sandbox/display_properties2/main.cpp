// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "support/entry/entry.h"
#include "vulkan_helpers/helper_functions.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::LibraryWrapper library_wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(CreateInstanceForApplication(
      data->allocator(), &library_wrapper, data,
      {"VK_KHR_display", "VK_KHR_get_display_properties2"}));

  containers::vector<VkPhysicalDevice> devices =
      vulkan::GetPhysicalDevices(data->allocator(), instance);

  for (const auto& dev : devices) {
    uint32_t display_properties_count;
    instance->vkGetPhysicalDeviceDisplayProperties2KHR(
        dev, &display_properties_count, nullptr);
    containers::vector<VkDisplayProperties2KHR> display_properties(
        display_properties_count,
        {VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR, nullptr},
        data->allocator());
    instance->vkGetPhysicalDeviceDisplayProperties2KHR(
        dev, &display_properties_count, display_properties.data());

    uint32_t plane_count;
    instance->vkGetPhysicalDeviceDisplayPlaneProperties2KHR(dev, &plane_count,
                                                            nullptr);

    for (const auto& display_prop : display_properties) {
      data->logger()->LogInfo("Display name: ",
                              display_prop.displayProperties.displayName);
      data->logger()->LogInfo("Persistent content: ",
                              display_prop.displayProperties.persistentContent);
      data->logger()->LogInfo(
          "Physical dimensions: ",
          display_prop.displayProperties.physicalDimensions.width, " ",
          display_prop.displayProperties.physicalDimensions.height);
      data->logger()->LogInfo(
          "Physical resolution: ",
          display_prop.displayProperties.physicalResolution.width, "",
          display_prop.displayProperties.physicalResolution.height);
      data->logger()->LogInfo(
          "Plane reorder possible: ",
          display_prop.displayProperties.planeReorderPossible);

      uint32_t mode_properties_count;
      instance->vkGetDisplayModeProperties2KHR(
          dev, display_prop.displayProperties.display, &mode_properties_count,
          nullptr);
      containers::vector<VkDisplayModeProperties2KHR> mode_properties(
          mode_properties_count,
          {VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR, nullptr},
          data->allocator());
      instance->vkGetDisplayModeProperties2KHR(
          dev, display_prop.displayProperties.display, &mode_properties_count,
          mode_properties.data());

      for (const auto& display_mode : mode_properties) {
        data->logger()->LogInfo(
            "Display mode: ",
            display_mode.displayModeProperties.parameters.visibleRegion.width,
            "x",
            display_mode.displayModeProperties.parameters.visibleRegion.height,
            " ", display_mode.displayModeProperties.parameters.refreshRate,
            " Hz");

        for (uint32_t plane_index = 0; plane_index < plane_count;
             plane_index++) {
          data->logger()->LogInfo("Plane: ", plane_index);

          VkDisplayPlaneInfo2KHR plane_info{
              VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR, nullptr,
              display_mode.displayModeProperties.displayMode, plane_index};

          VkDisplayPlaneCapabilities2KHR capabilities{
              VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR, nullptr};

          instance->vkGetDisplayPlaneCapabilities2KHR(dev, &plane_info,
                                                      &capabilities);

          data->logger()->LogInfo(
              "minSrcPosition: ", capabilities.capabilities.minSrcPosition.x,
              " ", capabilities.capabilities.minSrcPosition.y);
          data->logger()->LogInfo(
              "maxSrcPosition: ", capabilities.capabilities.maxSrcPosition.x,
              " ", capabilities.capabilities.maxSrcPosition.y);
          data->logger()->LogInfo(
              "minSrcExtent: ", capabilities.capabilities.minSrcExtent.width,
              " ", capabilities.capabilities.minSrcExtent.height);
          data->logger()->LogInfo(
              "maxSrcExtent: ", capabilities.capabilities.maxSrcExtent.width,
              " ", capabilities.capabilities.maxSrcExtent.height);
          data->logger()->LogInfo(
              "minDstPosition: ", capabilities.capabilities.minDstPosition.x,
              " ", capabilities.capabilities.minDstPosition.y);
          data->logger()->LogInfo(
              "maxDstPosition: ", capabilities.capabilities.maxDstPosition.x,
              " ", capabilities.capabilities.maxDstPosition.y);
          data->logger()->LogInfo(
              "minDstExtent: ", capabilities.capabilities.minDstExtent.width,
              " ", capabilities.capabilities.minDstExtent.height);
          data->logger()->LogInfo(
              "maxDstExtent: ", capabilities.capabilities.maxDstExtent.width,
              " ", capabilities.capabilities.maxDstExtent.height);
        }
      }
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
