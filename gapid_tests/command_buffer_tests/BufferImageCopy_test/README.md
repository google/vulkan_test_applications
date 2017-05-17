# vkCmdCopyBufferToImage / vkCmdCopyImageToBuffer

## Signatures
```c++
void vkCmdCopyBufferToImage(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions);

void vkCmdCopyImageToBuffer(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions);
```

# VkBufferImageCopy
```c++
typedef struct VkBufferImageCopy {
    VkDeviceSize                bufferOffset;
    uint32_t                    bufferRowLength;
    uint32_t                    bufferImageHeight;
    VkImageSubresourceLayers    imageSubresource;
    VkOffset3D                  imageOffset;
    VkExtent3D                  imageExtent;
} VkBufferImageCopy;
```

According to the Vulkan spec:
- `dstImageLayout` **must** be `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` or
    `VK_IMAGE_LAYOUT_GENERAL`
- `srcImageLayout` **must** be `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` or
    `VK_IMAGE_LAYOUT_GENERAL`
- `regionCount` **must** be >= 1
- `bufferOffset` **must** be a multiple of 4
- `bufferOffset` **must** be a multiple of the claling command's VkImage
    parameter's texel size
- `bufferRowLength` **must** be 0 or >= imageExtent.width
- `bufferImageHeight` **must** be 0 or >= imageExtent.height
- `imageOffset` must be >= `0, 0, 0`
- `imageOffset` + `imageExtent` must be within the image


These tests should test the following cases:
- [x] `dstImageLayout` == `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
- [ ] `dstImageLayout` == `VK_IMAGE_LAYOUT_GENERAL`
- [x] `srcImageLayout` == `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
- [ ] `srcImageLayout` == `VK_IMAGE_LAYOUT_GENERAL`
- [x] `regionCount` == 1
- [ ] `regionCount` > 1
- [x] `bufferOffset` == 0
- [ ] `bufferOffset` > 0
- [x] `bufferRowLength` == 0
- [ ] `bufferRowLength` > 0
- [x] `bufferImageHeight` == 0
- [ ] `bufferImageHeight` > 0
- [x] `imageOffset` == 0
- [ ] `imageOffset` > 0
- [x] `imageExtent` == image size
- [ ] `imageExtent` < image size