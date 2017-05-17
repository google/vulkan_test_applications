# vkGetPhysicalDeviceSurfaceSupportKHR / vkGetPhysicalDeviceSurfaceCapabilitiesKHR / vkGetPhysicalDeviceSurfaceFormatsKHR

## Signatures
```c++
VkResult vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    VkSurfaceKHR                                surface,
    VkBool32*                                   pSupported);

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice                            physicalDevice,
    VkSurfaceKHR                                surface,
    VkSurfaceCapabilitiesKHR*                   pSurfaceCapabilities);

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice                            physicalDevice,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pSurfaceFormatCount,
    VkSurfaceFormatKHR*                         pSurfaceFormats);

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice                            physicalDevice,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pPresentModeCount,
    VkPresentModeKHR*                           pPresentModes);
```

According to the Vulkan spec:
- vkGetPhysicalDeviceSurfaceSupportKHR
    - `physicalDevice` **must** be valid
    - `queueFamilyIndex` **must** be a valid queue family index
    - `surface` **must** be a valid surface
    - `supported` **must** be a valid bool pointer
- vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    - `physicalDevice` **must** be valid
    - `surface` **must** be valid
    - `pSurfaceCapabilities` **must** be valid
- vkGetPhysicalDeviceSurfaceFormatsKHR
    - `physicalDevice` **must** be valid
    - `surface` **must** be valid
    - `pSurfaceFormatCount` must be a valid pointer
    - `pSurfaceFormats` is either nullptr or a pointer to `*pSurfaceFormatCount`
            `VkSurfaceFormatKHR` structures.
- vkGetPhysicalDeviceSurfacePresentModesKHR
    - `physicalDevice` **must** be valid
    - `surface` **must** be valid
    - `pPresentModeCount` must be a valid pointer
    - `pPresentModes` is either nullptr or a pointer to `*pPresentModeCount`
            `VkPresentModeKHR` values.

## VkSurfaceCapabilitiesKHR
```c++
typedef struct VkSurfaceCapabilitiesKHR {
    uint32_t                         minImageCount;
    uint32_t                         maxImageCount;
    VkExtent2D                       currentExtent;
    VkExtent2D                       minImageExtent;
    VkExtent2D                       maxImageExtent;
    uint32_t                         maxImageArrayLayers;
    VkSurfaceTransformFlagsKHR       supportedTransforms;
    VkSurfaceTransformFlagBitsKHR    currentTransform;
    VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
    VkImageUsageFlags                supportedUsageFlags;
} VkSurfaceCapabilitiesKHR;
```

## VkSurfaceFormatKHR
```c++
typedef struct VkSurfaceFormatKHR {
    VkFormat           format;
    VkColorSpaceKHR    colorSpace;
} VkSurfaceFormatKHR;
```

## VkPresentModeKHR
```
typedef enum VkPresentModeKHR {
    VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
    VK_PRESENT_MODE_MAILBOX_KHR = 1,
    VK_PRESENT_MODE_FIFO_KHR = 2,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,
} VkPresentModeKHR;
```

These tests should test the following cases:
- [x] Valid `vkGetPhysicalDeviceSurfaceSupportKHR`
- [x] Valid `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`
- [x] `vkGetPhysicalDeviceSurfaceFormatsKHR:pSurfaceFormatCount` 0
- [x] `vkGetPhysicalDeviceSurfaceFormatsKHR:pSurfaceFormatCount` returned_value
- [x] `vkGetPhysicalDeviceSurfaceFormatsKHR:pSurfaceFormatCount`
        0 < val < returned_value
- [x] `pPresentModeCount` 0
- [x] `pPresentModeCount` returned_value
- [x] `pPresentModeCount` 0 < val < returned_value

