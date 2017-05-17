# vkCmdNextSubpass

## Signatures
```c++
void vkCmdNextSubpass(
    VkCommandBuffer                             commandBuffer,
    VkSubpassContents                           contents);
```

According to the Vulkan spec:
- The subpass index for a render pass begins at zero when `vkCmdBeginRenderPass`
  is recorded, and increments each time `vkCmdNextSubpass` is recorded.
- Moving to the next subpass automatically performs any multisample resolve
  operations in the subpass being ended. End-of-subpass multisample resolves
  are treated as color attachment writes for the purposes of synchronization.
- The current subpass index **must** be less than the number of subpasses in
  the render pass minus one
- This command **must** only be called inside of a render pass instance

These tests should test the following cases:
- [x] `content` of value `VK_SUBPASS_CONTENTS_INLINE`
- [ ] `content` of value `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`
