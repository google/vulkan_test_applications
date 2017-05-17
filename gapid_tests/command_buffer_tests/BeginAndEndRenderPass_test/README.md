# vkCmdBeginRenderPass / vkCmdEndRenderPass

## Signatures
```c++
void vkCmdBeginRenderPass(
    VkCommandBuffer                             commandBuffer,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                           contents);

void vkCmdEndRenderPass(
    VkCommandBuffer                             commandBuffer);
```

According to the Vulkan spec:
- vkCmdBeginRenderPass
  - `commandBuffer` **must** be a valid **primary** `VkCommandBuffer` handle
  - `pRenderPassBegin` **must** be a pointer to a valid `VkRenderPassBeginInfo`
    structure
  - `contents` **must** be one of the values: `VK_SUBPASS_CONTENTS_INLINE`
  and `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`
- vkCmdEndRenderPass
  - `commandBuffer` **must** be a valid primary `VkCommandBuffer` in recording
  state
  - The `VkCommandPool` that `commandBuffer` was allcoated from **must** support
  graphics operations
  - This command **must** only be called inside of a render pass instance
  - The current subpass index **must** be equal to the number of subpasses in
  the render pass minus one

## VkRenderPassBeginInfo
```c++
typedef struct VkRenderPassBeginInfo {
    VkStructureType        sType;
    const void*            pNext;
    VkRenderPass           renderPass;
    VkFramebuffer          framebuffer;
    VkRect2D               renderArea;
    uint32_t               clearValueCount;
    const VkClearValue*    pClearValues;
} VkRenderPassBeginInfo;
```

According to the Vulkan spec:
- VkCommandBufferBeginInfo
  - `sType` **must** be `VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO`
  - `pNext` **must** be `NULL`
  - `renderPass` **must** be a valid `VkRenderPass` handle
  - `framebuffer` **must** be a valid `VkFramebuffer` handle
  - If `clearValueCount` is not 0, `pClearValues` must be a pointer to an array
  of `clearValueCount` valid `VkClearValue` unions
  - Both of `framebuffer` and `renderPass` **must** have been created,
  allocated, or retrieved from the same `VkDevice`
  - `clearValueCount` **must** be greater than the largest attachment index in
  `renderPass` that specifies a `loadOp` (or `stencilLoadOp`, if the attachment
  has a depth/stencil format) of `VK_ATTACHMENT_LOAD_OP_CLEAR`

These tests should test the following cases:
- `contents` of valid values
  - [x] `VK_SUBPASS_CONTENTS_INLINE`
  - [ ] `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`
- `renderPass` of a valid handle
  - [x] None attachments
  - [x] Color attachments
  - [ ] Depth attachments
- `framebuffer` of a valid handle
  - [x] None attachments
  - [ ] Color and depth attachments
- [x] `renderArea` of different offsets and extents
- `clearValueCount` 0 or valid number
  - [x] 0 value `clearValueCount`
  - [x] `clearValueCount` is not 0, `pClearValues` should have corresponding
  clear values
- [x] `vkCmdEndRenderPass()` with a recording state primary `VkCommandBuffer`
handle.
