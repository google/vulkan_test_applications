# vkCmdClearAttachments

## Signatures
```c++
void vkCmdClearAttachments(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects);
```

# VkClearRect
```c++
typedef struct VkClearRect {
    VkRect2D    rect;
    uint32_t    baseArrayLayer;
    uint32_t    layerCount;
} VkClearRect;
```

# VkClearAttachment
```c++
typedef struct VkClearAttachment {
    VkImageAspectFlags    aspectMask;
    uint32_t              colorAttachment;
    VkClearValue          clearValue;
} VkClearAttachment;
```

According to the Vulkan spec:
- The rectangular region specified by a given element of `pRects` **must** be
  contained within the render area of the current render pass instance
- The layers specified by a given element of `pRects` **must** be contained
  within every attachment that `pAttachments` refers to
- `commandBuffer` **must** be in the recording state and this command **must**
  be called inside of a render pass instance
- `attachmentCount` **must** be greater than 0
- `rectCount` **must** be greater than 0
- If the `aspectMask` member of any given element of `pAttachments` contains
  `VK_IMAGE_ASPECT_COLOR_BIT`, the `colorAttachment` member of those elements
  **must** refer to a valid color attachment in the current subpass
- `colorAttachment` is only meaningful if `VK_IMAGE_ASPECT_COLOR_BIT` is set in
  `aspectMask`, in which case it is an index to the `pColorAttachments` array
  in the `VkSubpassDescription` structure of the current subpass which selectes
  the color attachment to clear. If `colorAttachment` is `VK_ATTACHMENT_UNUSED`
  then the clear has no effect.
- `aspectMast` **must not** include `VK_IMAGE_ASPECT_METADATA_BIT`

These tests should test the following cases:
- [x] Color attachment
- [ ] Depth attachment
- [ ] Stencil attachment
- [x] Single attachment
- [ ] Multiple attachments
- [x] Single layer image
- [ ] Multiple layers image
- [x] Single clear rectangular
- [ ] Multiple clear rectangulars
