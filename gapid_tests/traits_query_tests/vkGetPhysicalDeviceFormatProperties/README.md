# vkGetPhysicalDeviceFormatProperties

## Signatures
```c++
void vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkFormatProperties*                         pFormatProperties);
```

According to the Vulkan spec:
- `format` **must** be a valid `VkFormat` value
- `pFormatProperties` **must** be a pointer to a `VkFormatProperties` structure

## VkQueueFamilyProperties
```c++
typedef struct VkFormatProperties {
    VkFormatFeatureFlags    linearTilingFeatures;
    VkFormatFeatureFlags    optimalTilingFeatures;
    VkFormatFeatureFlags    bufferFeatures;
} VkFormatProperties;
```

These tests should test the following cases:
- [x] all valid `VkFormat` values
- [x] a variety of different formats
