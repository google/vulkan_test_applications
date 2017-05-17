# vkCreateImageView / vkDestroyImageView

## Signatures
```c++
VkResult vkCreateImageView(
    VkDevice                                    device,
    const VkImageViewCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImageView*                                pView);

void vkDestroyImageView(
    VkDevice                                    device,
    VkImageView                                 imageView,
    const VkAllocationCallbacks*                pAllocator);
```

# VkImageViewCreateInfo
```c++
typedef struct VkImageViewCreateInfo {
    VkStructureType            sType;
    const void*                pNext;
    VkImageViewCreateFlags     flags;
    VkImage                    image;
    VkImageViewType            viewType;
    VkFormat                   format;
    VkComponentMapping         components;
    VkImageSubresourceRange    subresourceRange;
} VkImageViewCreateInfo;
```

# VkComponentMapping
```c++
typedef struct VkComponentMapping {
    VkComponentSwizzle    r;
    VkComponentSwizzle    g;
    VkComponentSwizzle    b;
    VkComponentSwizzle    a;
} VkComponentMapping;
```

# VkImageSubresourceRange
```c++
typedef struct VkImageSubresourceRange {
    VkImageAspectFlags    aspectMask;
    uint32_t              baseMipLevel;
    uint32_t              levelCount;
    uint32_t              baseArrayLayer;
    uint32_t              layerCount;
} VkImageSubresourceRange;
```

According to the Vulkan spec:
- `sType` **must** be `VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO`
- `pNext` **must** be `NULL`
- `flags` **must** be 0
- `image` **must** be a valid `VkImage` handle
- `viewType` **must** be a valid `VKImageViewType` value and compaitable with
  the `image`
- `format` **must** be a valid `VkFormat` value and compaitable with the `image`
- `components` **must** be a valid `VkImageSubresourceRange` structure, in
  which each member (e.g. `r`) **must** be a valid `VkComponentSwizzle` value
  and compatible with the `image` and `format`.
- `subresourceRange` **must** be a valid `VkImageSubresourceRange` structure.
  - `aspectMask` **must** be a valid combination of `VkImageAspectFlagBits` and
  compatible with `format`. Its value **must not** be 0.
  - If `levelCount` is not `VK_REMAINING_MIP_LEVELS`, (`baseMipLevel` +
  `levelCount`) **must** be less than or equal to the `mipLevels` specified in
  `VkImageCreateInfo` when `image` was created
  - If `layerCount` is not `VK_REMAINING_ARRAY_LAYERS`, (`baseArrayLayer` +
  `layerCount`) **must** be less than or equal to the `arrayLayers` specified
  in `VkImageCreateInfo` when `image` was created

These tests should test the following cases:
- `image` created in different approach
  - [ ] Through `vkCreateImage`
  - [x] Through `vkCreateSwapchainKHR`
- `viewType` of valid values
  - [x] `VK_IMAGE_VIEW_TYPE_2D`
  - [ ] `VK_IMAGE_VIEW_TYPE_CUBE`
- `format` of valid `VkFormat` values
  - [x] `VK_FORMAT_R8G8B8A8_UNORM`
  - [ ] `VK_FORMAT_D16_UNORM`
  - [ ] `VK_FORMAT_R32_SINT`
- `components` memeber with different `VkComponentSwizzle` value
  - [x] `VK_COMPONENT_SWIZZLE_IDENTITY`
  - [ ] `VK_COMPONENT_SWIZZLE_R`
  - [ ] `VK_COMPONENT_SWIZZLE_G`
  - [ ] `VK_COMPONENT_SWIZZLE_B`
  - [ ] `VK_COMPONENT_SWIZZLE_A`
- `aspectMask` of valid `VkImageAspectFlags` values
  - [x] `VK_IMAGE_ASPECT_COLOR_BIT`
  - [ ] `VK_IMAGE_ASPECT_DEPTH_BIT`
- `baseMipLevel` and `levelCount` of valid values
  - [x] `baseMipLevel` of value 0 and `levelCount` of value 1
  - [ ] `baseMipLevel` of value 1 and `levelCount` of value 4
- `baseArrayLayer` and `layerCount` of valid values
  - [x] `baseArrayLayer` of value 0 and `layerCount` of value 1
  - [ ] `baseArrayLayer` of value 4 and `layerCount` of value 2
