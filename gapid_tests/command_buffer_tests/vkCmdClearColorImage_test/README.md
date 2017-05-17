# vkCmdClearColorImage

## Signatures
```c++
void vkCmdClearColorImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges);
```

# VkClearColorValue
```c++
typedef union VkClearColorValue {
    float       float32[4];
    int32_t     int32[4];
    uint32_t    uint32[4];
} VkClearColorValue;
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
- `image` **must** have been created with `VK_IMAGE_USAGE_TRANSFER_DST_BIT`
  usage flag
- `imageLayout` **must** specify the layout of the image subresource ranges of
  `image` specified in the `pRanges` at the time this command is executed on a
  `VkDevice`
- `imageLayout` **must** be either of `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` or
  `VK_IMAGE_LAYOUT_GENERAL`
- The image range of any given element of `pRanges` **must** be an image
  subresource range that is contained within `image`
- `image` **must** not have a compressed or depth/stencil format
- `rangeCount` **must** be greater than 0
- The `aspectMask` of all image subresource ranges **must** only include
  `VK_IMAGE_ASPECT_COLOR_BIT`

These tests should test the following cases:
- [x] `rangeCount` of value 1
- [ ] `rangeCount` of value more than 1
- [x] `imageLayout` of `VK_IMAEG_LAYOUT_TRANSFER_DST_OPTIMAL`
- [ ] `imageLayout` of `VK_IMAEG_LAYOUT_GENERAL`
- [x] `baseMipLevel` of value 0
- [ ] `baseMipLevel` of value other than 0
- [x] `levelCount` of value 1
- [ ] `levelCount` of value more than 1
- [x] `baseArrayLayer` of value 0
- [ ] `baseArrayLayer` of value other than 0
- [x] `layerCount` of value 1
- [ ] `layerCount` of value more than 1
