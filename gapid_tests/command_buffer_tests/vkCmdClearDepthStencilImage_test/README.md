# vkCmdClearDepthStencilImage

## Signatures
```c++
void vkCmdClearDepthStencilImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges);
```

# VkClearDepthStencilValue
```c++
typedef union VkClearDepthStencilValue {
    float       depth;
    uint32_t    stencil;
} VkClearDepthStencilValue;
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
- `image` **must** have a depth/stencil format
- `imageLayout` **must** specify the layout of the image subresource ranges of
  `image` specified in the `pRanges` at the time this command is executed on a
  `VkDevice`
- `imageLayout` **must** be either of `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` or
  `VK_IMAGE_LAYOUT_GENERAL`
- The image range of any given element of `pRanges` **must** be an image
  subresource range that is contained within `image`
- `rangeCount` **must** be greater than 0
- The `aspectMask` of each image subresource ranges **can** include
  `VK_IMAGE_ASPECT_DEPTH_BIT` if the image format has a depth component, and
  `VK_IMAGE_ASPECT_STENCIL_BIT` if the image format has a stencil component.

These tests should test the following cases:
- [x] Clear depth data with `VK_IMAGE_ASPECT_DEPTH_BIT`
- [ ] Clear stencil data with `VK_IMAGE_ASPECT_STENCIL_BIT`
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
