# vkCreateFramebuffer / vkDestroyFramebuffer

## Signatures
```c++
VkResult vkCreateFramebuffer(
    VkDevice                                    device,
    const VkFramebufferCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFramebuffer*                              pFramebuffer);
void vkDestroyFramebuffer(
    VkDevice                                    device,
    VkFramebuffer                               framebuffer,
    const VkAllocationCallbacks*                pAllocator);
```

## VkFramebufferCreateInfo
```c++
typedef struct VkFramebufferCreateInfo {
    VkStructureType             sType;
    const void*                 pNext;
    VkFramebufferCreateFlags    flags;
    VkRenderPass                renderPass;
    uint32_t                    attachmentCount;
    const VkImageView*          pAttachments;
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    layers;
} VkFramebufferCreateInfo;
```

According to the Vulkan spec:
- `device` **must** be vald
- `pCreateInfo` **must** point to a valid `VkFramebufferCreateInfo` structure.
- `sType` **must** be `VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO`
- `pNext` **must** be null
- `flags` **must** be 0
- `renderPass` is a valid renderPass
- `attachmentCount` can be 0 or > 0
- `renderPass` and `Attachments` must have come from the same device
- `attachmentCount` must be the same as specified in `renderPass`


These tests should test the following cases:
- [ ] `attachmentCount` == 0
- [x] `attachmentCount` == 1
- [ ] `attachmentCount` > 1
