# vkCreateInstance / vkDestroyInstance

## Signatures
```c++
VkResult vkEnumeratePhysicalDevices(
    VkInstance                                  instance,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices);
```

These tests should test the following cases:
- [x] `pPhysicalDeviceCount` == 0
- [x] `pPhysicalDeviceCount` == the number returned by first call
- [x] `pPhysicalDeviceCount` == 0, pPhysicalDevices != nullptr
- [x] `pPhysicalDeviceCount` < returned number from first call