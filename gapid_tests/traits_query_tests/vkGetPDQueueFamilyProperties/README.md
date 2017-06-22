# vkGetPhysicalDeviceQueueFamilyProperties

## Signatures
```c++
void vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice                            physicalDevice,
    uint32_t*                                   pQueueFamilyPropertyCount,
    VkQueueFamilyProperties*                    pQueueFamilyProperties);
```

## VkQueueFamilyProperties
```c++
typedef struct VkQueueFamilyProperties {
    VkQueueFlags    queueFlags;
    uint32_t        queueCount;
    uint32_t        timestampValidBits;
    VkExtent3D      minImageTransferGranularity;
} VkQueueFamilyProperties;
```

These tests should test the following cases:
- [x] `pQueueFamilyProperties` == nullptr
- [x] `pQueueFamilyProperties` != nullptr
  - [x] `pQueueFamilyPropertyCount` == 0
  - [x] `pQueueFamilyPropertyCount` < the number returned by first call
  - [x] `pQueueFamilyPropertyCount` == the number returned by first call
  - [x] `pQueueFamilyPropertyCount` > the number returned by first call
