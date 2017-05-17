# vkCmdResolveImage

## Signatures
```c++
void vkCmdResolveImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions);
```

# VkImageResolve
```c++
typedef struct VkImageResolve {
    VkImageSubresourceLayers    srcSubresource;
    VkOffset3D                  srcOffset;
    VkImageSubresourceLayers    dstSubresource;
    VkOffset3D                  dstOffset;
    VkExtent3D                  extent;
} VkImageResolve;
```

According to the Vulkan spec:
- This command **must** only be called **outside** of a render pass instance
- `srcImage` **must** have a sample count equal to any valid sample count value
  other than `VK_SAMPLE_COUNT_1_BIT`
- `dstImage` **must** have a sample count equal to `VK_SAMPLE_COUNT_1_BIT`
- If `dstImage` was created with `tiling` equal to `VK_IMAGE_TILING_LINEAR`,
  `dstImage` **must** have been created with a `format` that supports being a
  color attachment, as specified by the `VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT`
  flag in `VkFormatProperties::linearTilingFeatures` returned by
  `vkGetPhysicalDeviceFormatProperties`
- If `dstImage` was created with `tiling` equal to `VK_IMAGE_TILING_OPTIMAL`,
  `dstImage` **must** have been created with a `format` that supports being a
  color attachment, as specified by the `VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT`
  flag in `VkFormatProperties::optimalTilingFeatures` returned by
  `vkGetPhysicalDeviceFormatProperties`
- `srcImageLayout` **must** be either of `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
  or `VK_IMAGE_LAYOUT_GENERAL`
- `dstImageLayout` **must** be either of `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
  or `VK_IMAGE_LAYOUT_GENERAL`
- `commandBuffer` **must** be in the recording state
- `pRegions` **must** be a pointer to an array of `regionCount` valid
  `VkImageResolve` structures
- `regionCount` **must** be greater than 0
- `aspectMask` member of `srcSubresource` and `dstSubresource` **must** only
  contain `VK_IMAGE_ASPECT_COLOR_BIT`
- The `layerCount` member of `srcSubresource` and `dstSubresource` **must**
  match
- If either of the calling command's `srcImage` or `dstImage` parameters are of
  `VkImageType VK_IMAGE_TYPE_3D`, the `baseArrayLayer` and `layerCount` members
  of both `srcSubresource` and `dstSubresource` **must** be 0 and 1,
  respectively
- `srcOffset.x` and `(extent.width + srcOffset.x)` **must** both be greater than
  or equal to 0 and less than or equal to the source image subresource width
- `srcOffset.y` and `(extent.height + srcOffset.y)` **must** both be greater
  than or equal to 0 and less than or equal to the source image subresource
  height
- If the calling command's `srcImage` is of type `VK_IMAGE_TYPE_1D`, then
  `srcOffset.y` **must** be 0 and `extent.height` **must** be 1
- `srcOffset.z` and `(extent.depth + srcOffset.z)` **must** both be greater
  than or equal to 0 and less than or equal to the source image subresource
  depth
- If the calling command's `srcImage` is of type `VK_IMAGE_TYPE_1D` or
  `VK_IMAGE_TYPE_2D`, then `srcOffset.z` **must** be 0 and `extent.depth`
  **must** be 1
- `dstOffset.x` and `(extent.width + dstOffset.x)` **must** both be greater than
  or equal to 0 and less than or equal to the source image subresource width
- `dstOffset.y` and `(extent.height + dstOffset.y)` **must** both be greater
  than or equal to 0 and less than or equal to the source image subresource
  height
- If the calling command's `dstImage` is of type `VK_IMAGE_TYPE_1D`, then
  `dstOffset.y` **must** be 0 and `extent.height` **must** be 1
- `dstOffset.z` and `(extent.depth + dstOffset.z)` **must** both be greater
  than or equal to 0 and less than or equal to the source image subresource
  depth
- If the calling command's `dstImage` is of type `VK_IMAGE_TYPE_1D` or
  `VK_IMAGE_TYPE_2D`, then `dstOffset.z` **must** be 0 and `extent.depth`
  **must** be 1

These tests should test the following cases:
- [x] 2D type image
- [ ] 3D type image
- [x] `layerCount` of value 1
- [ ] `layerCount` of value other than 1
- [x] `srcOffset` of value 0 in all dimensions
- [ ] `srcOffset` of value other than 0 in all dimensions
- [x] `dstOffset` of value 0 in all dimensions
- [ ] `dstOffset` of value other than 0 in all dimensions
- [x] `dstImage` with `tiling` equal to `VK_IMAGE_TILING_OPTIMAL`
- [ ] `dstImage` with `tiling` equal to `VK_IMAGE_TILING_LINEAR`
- [x] one region
- [ ] multiple regions
