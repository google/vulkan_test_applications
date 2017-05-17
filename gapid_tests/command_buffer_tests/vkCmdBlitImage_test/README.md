# vkCmdBlitImage

## Signatures
```c++
void vkCmdBlitImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter);
```

# VkImageBlit
```c++
typedef struct VkImageBlit {
    VkImageSubresourceLayers    srcSubresource;
    VkOffset3D                  srcOffsets[2];
    VkImageSubresourceLayers    dstSubresource;
    VkOffset3D                  dstOffsets[2];
} VkImageBlit;
```

According to the Vulkan spec:
- Integer format can **only** be converted to other integer format with the
  same signedness
- Format conversions on unorm, snorm, unscaled and packed float formats of the
  copied aspect of the image are performed by first converting the pixels to
  float values
- For sRGB source formats, nonlinear RGB values are converted to linear
  representation prior to filtering
- After filtering, the float values are first clamped and then cast to the
  destination image format. In case of sRGB destination format, linear RGB
  values are converted to nonlinear representation before writing the pixel to
  the image
- The union of all destination regions, specified by the elements of `pRegions`,
  must not overlap in memory with any texel that may be sampled during the blit
  operation
- `srcImage` **must** have been created with `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`
  usage flag.
- `dstImage` **must** have been created with `VK_IMAGE_USAGE_TRANSFER_DST_BIT`
  usage flag.
- `srcImageLayout` **must** be `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` or
    `VK_IMAGE_LAYOUT_GENERAL`
- `dstImageLayout` **must** be `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` or
  `VK_IMAGE_LAYOUT_GENERAL`
- If either of `srcImage` or `dstImage` was created with a signed integer
  `VkFormat`, the other **must** also have been created with the a signed
  integer `VKFormat`
- If either of `srcImage` or `dstImage` was created with a unsigned integer
  `VkFormat`, the other **must** also have been created with the a unsigned
  integer `VKFormat`
- If either of `srcImage` or `dstImage` was created with a depth/stencil format,
  the other **must** have exactly the same format
- `srcImage`/`dstImage` **must** use a format that support
  `VK_FORMAT_FEATURE_BLIT_DST_BIT`/`VK_FORMAT_FEATURE_BLIT_DST_BIT` respectively
- Both `srcImage` and `dstImage` **must** have been created with a `samples`
  value of `VK_SAMPLE_COUNT_1_BIT`
- `regionCount` **must** be greater than 0
- `commandBuffer` **must** be in the recording state
- Each of `commandBuffer`, `dstImage` and `srcImage` **must** have been created,
  allocated, or retrieved from the same `VkDevice`
- If `filter` is `VK_FILTER_LINEAR`, `srcImage` **must** be of a format which
  supports linear filtering, as specified by the
  `VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT` flag


These tests should test the following cases:
- [x] Blit to an exactly the same color image
- [ ] Blit to an image with different dimensions
- [ ] Blit to an image with different format
- [ ] Blit a depth/stencil image
- [x] `filter` of value `VK_FILTER_LINEAR`
- [ ] `filter` of value `VK_FILTER_NEAREST`
