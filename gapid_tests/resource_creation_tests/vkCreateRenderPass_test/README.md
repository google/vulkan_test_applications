# vkCreateRenderPass / vkDestroyRenderPass

## Signatures
```c++
VkResult vkCreateRenderPass(
    VkDevice                                    device,
    const VkRenderPassCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass);
void vkDestroyRenderPass(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    const VkAllocationCallbacks*                pAllocator);
```

## VkRenderPassCreateInfo
```c++
typedef struct VkRenderPassCreateInfo {
    VkStructureType                   sType;
    const void*                       pNext;
    VkRenderPassCreateFlags           flags;
    uint32_t                          attachmentCount;
    const VkAttachmentDescription*    pAttachments;
    uint32_t                          subpassCount;
    const VkSubpassDescription*       pSubpasses;
    uint32_t                          dependencyCount;
    const VkSubpassDependency*        pDependencies;
} VkRenderPassCreateInfo;
```
## VkAttachmentDescription
```c++
typedef struct VkAttachmentDescription {
    VkAttachmentDescriptionFlags    flags;
    VkFormat                        format;
    VkSampleCountFlagBits           samples;
    VkAttachmentLoadOp              loadOp;
    VkAttachmentStoreOp             storeOp;
    VkAttachmentLoadOp              stencilLoadOp;
    VkAttachmentStoreOp             stencilStoreOp;
    VkImageLayout                   initialLayout;
    VkImageLayout                   finalLayout;
} VkAttachmentDescription;
```

## VkSubpassDescription
```c++
typedef struct VkSubpassDescription {
    VkSubpassDescriptionFlags       flags;
    VkPipelineBindPoint             pipelineBindPoint;
    uint32_t                        inputAttachmentCount;
    const VkAttachmentReference*    pInputAttachments;
    uint32_t                        colorAttachmentCount;
    const VkAttachmentReference*    pColorAttachments;
    const VkAttachmentReference*    pResolveAttachments;
    const VkAttachmentReference*    pDepthStencilAttachment;
    uint32_t                        preserveAttachmentCount;
    const uint32_t*                 pPreserveAttachments;
} VkSubpassDescription;
```

## VkAttachmentReference
```c++
typedef struct VkAttachmentReference {
    uint32_t         attachment;
    VkImageLayout    layout;
} VkAttachmentReference;
```

## VkSubpassDependency
```c++
typedef struct VkSubpassDependency {
    uint32_t                srcSubpass;
    uint32_t                dstSubpass;
    VkPipelineStageFlags    srcStageMask;
    VkPipelineStageFlags    dstStageMask;
    VkAccessFlags           srcAccessMask;
    VkAccessFlags           dstAccessMask;
    VkDependencyFlags       dependencyFlags;
} VkSubpassDependency;
```

According to the Vulkan spec:
- `device` **must** be vald
- `pCreateInfo` **must** point to a valid `VkRenderPassCreateInfo` structure.
- `sType` **must** be `VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO`
- `pNext` **must** be null
- `flags` **must** be 0
- `attachmentCount` is the number of attachments
- `pAttachments` is a pointer to `attachmentCount` structures or `NULL` if
attachmentCount is 0
- `subpassCount` is the number of subpasses to create must be >= 1
- `pSubpasses` is a pointer to `subpassCount` `vkSubpassDescription` structs.
- `dependencyCount` is the number of dependencies  between subpasses
- `pDependencies` is a pointer to `dependencyCount` `VkSubpassDependency`
structs.
- `VkAttachmentDescription`
  - all parameters must be valid
- `VkSubpassDescription`
  - flags **must** be 0
  - `pipelineBindPoint` must be a valid `VkPipelineBindPoint`
  - `pInputAttachments` points to `inputAttachmentCount`
        `VkAttachmentReference` structures
  - `pColorAttachments` points to `colorAttachmentCount`
         `VkAttachmentReference` structures
  - `pResolveAttachments` is either `NULL` or points to `colorAttachmentCount`
         `VkAttachmentReference` structures
  - `pDepthStencilAttachment` is either `NULL` or points to a
        `VkAttachmentReference` structure
  - `pPreserveAttachments` is either `NULL` or points to
        `preserveAttachmentCount` `uint32_t*`
- `VkSubpassDependency`
  - `srcSubpass` **must** be an index of a valid subpass (< subpassCount)
  - `dstSubpass` **must** be an index of a valid subpass (< subpassCount)
  - `srcStageMask` **must not** be 0
  - `dstStageMask` **must not** be 0
  - `srcAccessMask` **must** be a combination of `VkAccessFlagsBits`
  - `dstAccessMask` **must** be a combination of `VkAccessFlagsBits`
  - `dependencyFlags` **must** be a valid combination of `VkDependencyFlagBits`


These tests should test the following cases:
- [x] `attachmentCount` == 0
- [x] `attachmentCount` > 0
- [x] `subPassCount` == 1
- [ ] `subPassCount` > 1
- [ ] `dependencyCount` == 0
- [ ] `dependencyCount` > 0
- [ ] `inputAttachmentCount` == 0
- [ ] `inputAttachmentCount` > 0
- [x] `colorAttachmentCount` == 0
- [x] `colorAttachmentCount` > 0
- [ ] `pResolveAttachments` == nullptr && colorAttachmentCount == 0
- [ ] `pResolveAttachments` == nullptr && colorAttachmentCount != 0
- [ ] `pResolveAttachments` != nullptr && colorAttachmentCount > 0
- [ ] `pDepthStencilAttachment` == nullptr
- [ ] `pDepthStencilAttachment` != nullptr
- [ ] `preserveAttachmentCount` == 0
- [ ] `preserveAttachmentCount` > 0
- [x] `srcSubpass` == 0
- [ ] `srcSubpass` == MaxSubpass

