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

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  containers::vector<VkPhysicalDevice> devices(
      vulkan::GetPhysicalDevices(data->root_allocator, instance));

  uint32_t num_layers = 0;
  LOG_EXPECT(==, data->log, VK_SUCCESS,
             wrapper.vkEnumerateInstanceLayerProperties(&num_layers, nullptr));
  containers::vector<VkLayerProperties> layer_properties(data->root_allocator);
  layer_properties.resize(num_layers);

  if (num_layers > 1) {
    uint32_t num_reduced_layers = num_layers - 1;
    LOG_EXPECT(==, data->log, VK_INCOMPLETE,
               wrapper.vkEnumerateInstanceLayerProperties(
                   &num_reduced_layers, layer_properties.data()));
  }

  LOG_EXPECT(==, data->log, VK_SUCCESS,
             wrapper.vkEnumerateInstanceLayerProperties(
                 &num_layers, layer_properties.data()));

  uint32_t num_extension_properties = 0;

  LOG_EXPECT(==, data->log, VK_SUCCESS,
             wrapper.vkEnumerateInstanceExtensionProperties(
                 nullptr, &num_extension_properties, nullptr));

  containers::vector<VkExtensionProperties> extension_properties(
      data->root_allocator);
  extension_properties.resize(num_extension_properties);

  if (num_extension_properties > 1) {
    uint32_t num_reduced_extension_properties = num_extension_properties - 1;
    LOG_EXPECT(==, data->log, VK_INCOMPLETE,
               wrapper.vkEnumerateInstanceExtensionProperties(
                   nullptr, &num_reduced_extension_properties,
                   extension_properties.data()));
  }

  LOG_EXPECT(
      ==, data->log, VK_SUCCESS,
      wrapper.vkEnumerateInstanceExtensionProperties(
          nullptr, &num_extension_properties, extension_properties.data()));

  for (const auto& extension : extension_properties) {
    data->log->LogInfo("Instance Extension Found ", extension.extensionName);
  }

  for (const auto& layer : layer_properties) {
    data->log->LogInfo("Instance Layer Found ", layer.layerName);

    LOG_EXPECT(==, data->log, VK_SUCCESS,
               wrapper.vkEnumerateInstanceExtensionProperties(
                   layer.layerName, &num_extension_properties, nullptr));

    extension_properties.resize(num_extension_properties);
    LOG_EXPECT(==, data->log, VK_SUCCESS,
               wrapper.vkEnumerateInstanceExtensionProperties(
                   layer.layerName, &num_extension_properties,
                   extension_properties.data()));
    for (const auto& extension : extension_properties) {
      data->log->LogInfo("  Extension Found ", extension.extensionName);
    }
  }

  size_t device_count = 0;
  for (const auto& physical_device : devices) {
    data->log->LogInfo("PhysicalDevice ", device_count++);
    LOG_EXPECT(==, data->log, VK_SUCCESS,
               instance->vkEnumerateDeviceLayerProperties(
                   physical_device, &num_layers, nullptr));
    layer_properties.resize(num_layers);

    if (num_layers > 1) {
      uint32_t num_reduced_layers = num_layers - 1;
      LOG_EXPECT(
          ==, data->log, VK_INCOMPLETE,
          instance->vkEnumerateDeviceLayerProperties(
              physical_device, &num_reduced_layers, layer_properties.data()));
    }

    LOG_EXPECT(==, data->log, VK_SUCCESS,
               instance->vkEnumerateDeviceLayerProperties(
                   physical_device, &num_layers, layer_properties.data()));

    LOG_EXPECT(
        ==, data->log, VK_SUCCESS,
        instance->vkEnumerateDeviceExtensionProperties(
            physical_device, nullptr, &num_extension_properties, nullptr));

    extension_properties.resize(num_extension_properties);

    if (num_extension_properties > 1) {
      uint32_t num_reduced_extension_properties = num_extension_properties - 1;
      LOG_EXPECT(
          ==, data->log, VK_INCOMPLETE,
          instance->vkEnumerateDeviceExtensionProperties(
              physical_device, nullptr, &num_reduced_extension_properties,
              extension_properties.data()));
    }

    LOG_EXPECT(==, data->log, VK_SUCCESS,
               instance->vkEnumerateDeviceExtensionProperties(
                   physical_device, nullptr, &num_extension_properties,
                   extension_properties.data()));

    for (const auto& extension : extension_properties) {
      data->log->LogInfo("  Device Extension Found ", extension.extensionName);
    }

    for (const auto& layer : layer_properties) {
      data->log->LogInfo("  Device Layer Found ", layer.layerName);

      LOG_EXPECT(==, data->log, VK_SUCCESS,
                 instance->vkEnumerateDeviceExtensionProperties(
                     physical_device, layer.layerName,
                     &num_extension_properties, nullptr));

      extension_properties.resize(num_extension_properties);
      LOG_EXPECT(==, data->log, VK_SUCCESS,
                 instance->vkEnumerateDeviceExtensionProperties(
                     physical_device, layer.layerName,
                     &num_extension_properties, extension_properties.data()));
      for (const auto& extension : extension_properties) {
        data->log->LogInfo("    Extension Found ", extension.extensionName);
      }
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
