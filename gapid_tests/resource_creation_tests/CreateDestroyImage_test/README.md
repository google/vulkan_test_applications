# vkCreateImage / vkDestroyImage

## Signatures
```c++
VkResult vkCreateImage(
    VkDevice                                    device,
    const VkImageCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImage*                                    pImage);

void vkDestroyImage(
    VkDevice                                    device,
    VkImage                                     image,
    const VkAllocationCallbacks*                pAllocator);
```

# VkImageCreateInfo
```c++
typedef struct VkImageCreateInfo {
    VkStructureType          sType;
    const void*              pNext;
    VkImageCreateFlags       flags;
    VkImageType              imageType;
    VkFormat                 format;
    VkExtent3D               extent;
    uint32_t                 mipLevels;
    uint32_t                 arrayLayers;
    VkSampleCountFlagBits    samples;
    VkImageTiling            tiling;
    VkImageUsageFlags        usage;
    VkSharingMode            sharingMode;
    uint32_t                 queueFamilyIndexCount;
    const uint32_t*          pQueueFamilyIndices;
    VkImageLayout            initialLayout;
} VkImageCreateInfo;
```

According to the Vulkan spec:
- `sType` **must** be `VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO`
- `pNext` **must** be `NULL`
- `flags` **must** be a valid combination of `VkImageCreateFlagBits` values
- `usage` **must not** be 0
- `format` **must not** be `VK_FORMAT_UNDEFINED`
- `width`, `height` and `depth` members of `extent` must all be greater than 0
- `mipLevels` **must** be greater than 0 and less than or equal to
  `floor(log(max(extent.width, extent.height, extent.depth))/log(2)) + 1`
- `arrayLayers` **must** be greater than 0
- `initialLayout` **must** be `VK_IMAGE_LAYOUT_UNDEFINED` or
  `VK_IMAGE_LAYOUT_PREINITIALIZED`
- The combination of `format`, `imageType`, `flags`, `samples`, `usage` and
  `tiling` **must** be valid, refer to Vulkan spec for details.

These tests should test the following cases:
- `flags` of valid `VkImageCreateFlagBits` values
  - [x] zero value (0)
  - [x] `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT`
- [x] `imageType` of `VK_IMAGE_TYPE_1D`, `VK_IMAGE_TYPE_2D` or `VK_IMAGE_TYPE_3D`
- `format` of valid `VkFormat` values
  - [x] `VK_FORMAT_R8G8B8A8_UNORM`
  - [x] `VK_FORMAT_D16_UNORM`
- `extent` of valid `VkExtent3D` values
  - [x] 1D type image
  - [x] 2D type image
  - [x] 3D type image
- `mipLevels` 1 or greater
  - [x] of value 1
  - [x] of value 5
- `arrayLayers` of valid values
  - [x] of value 1
  - [x] of value 6
- `samples` of valid `VkSampleCountFlagBits` values
  - [x] `VK_SAMPLE_COUNT_1_BIT`
  - [x] `VK_SAMPLE_COUNT_4_BIT`
- [x] `tiling` of `VK_IMAGE_TILING_OPTIMAL` or `VK_IMAGE_TILING_LINEAR`
- `usage` of valid `VkImageUsageFlagBits` values
  - [x] `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`
  - [x] `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTAChMENT_BIT`
  - [x] `VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT`
- `sharingMode` of valid mode with vaild values of `queueFamilyIndexCount` and
  `pQueueFamilyIndices`
  - [x] `sharingMode` is set to `VK_SHARING_MODE_CONCURRENT`
  - [ ] `sharingMode` is set to `VK_SHARING_MODE_EXCLUSIVE`
- [x] `vkDestroyImage` can be called correctly
