# vkCreateSwapchainKHR / vkDestroySwapchainKHR

## Signatures
```c++
VkResult vkCreateSwapchainKHR(
    VkDevice                                    device,
    const VkSwapchainCreateInfoKHR*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSwapchainKHR*                             pSwapchain);

void vkDestroySwapchainKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    const VkAllocationCallbacks*                pAllocator);
```

# VkSwapchainCreateInfoKHR
```c++
typedef struct VkSwapchainCreateInfoKHR {
    VkStructureType                  sType;
    const void*                      pNext;
    VkSwapchainCreateFlagsKHR        flags;
    VkSurfaceKHR                     surface;
    uint32_t                         minImageCount;
    VkFormat                         imageFormat;
    VkColorSpaceKHR                  imageColorSpace;
    VkExtent2D                       imageExtent;
    uint32_t                         imageArrayLayers;
    VkImageUsageFlags                imageUsage;
    VkSharingMode                    imageSharingMode;
    uint32_t                         queueFamilyIndexCount;
    const uint32_t*                  pQueueFamilyIndices;
    VkSurfaceTransformFlagBitsKHR    preTransform;
    VkCompositeAlphaFlagBitsKHR      compositeAlpha;
    VkPresentModeKHR                 presentMode;
    VkBool32                         clipped;
    VkSwapchainKHR                   oldSwapchain;
} VkSwapchainCreateInfoKHR;
```


According to the Vulkan spec:
- `pCreateInfo` **must** be a pointer to a valid `VkSwapchainCreateInfoKHR`
structure
- `flags` **must** be 0
- `imageFormat` **must** be valid
- `imageColorSpace` **must** be valid
- `imageUsage` **must** be a valid combination of `VkImageUsageFlagBits` values
- `preTransform` **must** be valid
- `compositeAlpha` **must** be valid
- `oldSwapchain` is either null OR a valid `VkSwapchainKHR`
- `surface` **must** be a supported device as given by
`vkGetPhysicalDeviceSurfaceSupportKHR`
- The native window that `surface` **must not** be associated with a swapchain
that is not `oldSwapchain`
- `minImageCount` **must** be >= `minImageCount` from `VkSurfaceCapabilitiesKHR`
- `minImageCount` **must** be <= `maxImageCount` from `VkSurfaceCapabilitiesKHR`
- `imageFormat` and `imageColorSpace` **must** match at least one
        of `vkGetPhysicalDeviceSurfaceFormatsKHR`
- `imageExtent` **must** be between `minImageExtent` and `maxImageExtent` from
  `VkSurfaceCapabilitiesKHR`
- `imageArrayLayers` **must** > 0 && < `maxImageLayers` from
        `VkSurfaceCapabilitiesKHR`
- `imageUsage` **must** a subset of `supportedUsageFlags` of
        `VkSurfaceCapabilitiesKHR`
- If `imageSharingMode` is `VK_SHARING_MODE_CONCURRENT` `pQueueFamilyIndices` &&
        `queueFamilyIndexCount` **must** be valid
- `preTransform` **must** be one of the `supportedTransforms` in
        `VkSurfaceCapabilitiesKHR`
- `compositeAlpha` **must** be one of the `supportedCompositeAlpha` in
        `VkSurfaceCapabilitiesKHR`
- `presentMode` **must** be one of the values returned by
        `vkGetPhysicalDeviceSurfacePresentModesKHR`


These tests should test the following cases:
- [x] `oldSwapchain` is null
- [ ] `oldSwapchain` is not null
- [ ] `minImageCount` is min or > min
- [x] `queueFamilyIndexCount` > 0
- [ ] `imageFormat` is first or not first format
- [ ] `imageExtent` is min or maxImageCount
- [x] `imageSharingMode` > 1 queue.
- [ ] `preTransform` is identity or not
- [ ] `presentMode` is first or != first present mode