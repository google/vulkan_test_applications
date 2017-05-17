# Physical Device Image Format Traits

## Signatures
```c++
VkResult vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkImageFormatProperties*                    pImageFormatProperties);

void vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkImageType                                 type,
    VkSampleCountFlagBits                       samples,
    VkImageUsageFlags                           usage,
    VkImageTiling                               tiling,
    uint32_t*                                   pPropertyCount,
    VkSparseImageFormatProperties*              pProperties);
```

For `vkGetPhysicalDeviceImageFormatProperties`, according to the Vulkan spec:
- `usage` **must not** be 0

For `vkGetPhysicalDeviceSparseImageFormatProperties`, according to
the Vulkan spec:
- `usage` **must not** be 0
- `samples` must be a bit value that is set in
  `VkImageFormatProperties::sampleCounts` returned by
  `vkGetPhysicalDeviceImageFormatProperties` with `format`, `type`, `tiling`,
  and `usage` equal to those in this command and `flags` equal to the value
  that is set in `VkImageCreateInfo::flags` when the image is created

## VkQueueFamilyProperties
```c++
typedef struct VkImageFormatProperties {
    VkExtent3D            maxExtent;
    uint32_t              maxMipLevels;
    uint32_t              maxArrayLayers;
    VkSampleCountFlags    sampleCounts;
    VkDeviceSize          maxResourceSize;
} VkImageFormatProperties;
```

## VkSparseImageFormatProperties
```c++
typedef struct VkSparseImageFormatProperties {
    VkImageAspectFlags          aspectMask;
    VkExtent3D                  imageGranularity;
    VkSparseImageFormatFlags    flags;
} VkSparseImageFormatProperties;
```

For `vkGetPhysicalDeviceImageFormatProperties`, these tests should test
the following cases:
- [x] all valid `VkFormat` values
- [x] all valid `VkImageType` values
- [x] all valid `VkImageTiling` values
- [x] all valid `VkImageUsageFlags` values excluding 0
- [x] all valid `VkImageCreateFlags` values

For `vkGetPhysicalDeviceSparseImageFormatProperties`, these tests should test
the following cases:
- [ ] All combinations of
  - all valid `VkFormat` values
  - all valid `VkImageType` values
  - all valid `VkSampleCountFlagBits` values
  - all valid `VkImageUsageFlags` values excluding 0
  - all valid `VkImageTiling` values
- [ ] And for each of the above combinations:
  - [ ] `pProperties` == nullptr
  - [ ] `pProperties` != nullptr
    - [ ] `pPropertyCount` == 0
    - [ ] `pPropertyCount` < the number returned by first call
    - [ ] `pPropertyCount` == the number returned by first call
    - [ ] `pPropertyCount` > the number returned by first call
