# vkCmdSetStencilCompareMask / vkCmdSetStencilWriteMask

## Signatures
```c++
void vkCmdSetStencilCompareMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    compareMask);

void vkCmdSetStencilWriteMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    writeMask);
```

According to the Vulkan spec:
- `vkCmdSetStencilCompareMask`:
  - The currently bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK` dynamic state enabled.
- `vkCmdSetStencilWriteMask`:
  - The currently bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_STENCIL_WRITE_MASK` dynamic state enabled.
- `faceMask` **must** be a valid combination of `VkStencilFaceFlagBits` values
- `faceMask` must not be 0
- `commandBuffer` **must** be in the recording state

These tests should test the following cases:
- `flagMask` of valid value:
  - [x] `VK_STENCIL_FACE_FRONT_BIT`
  - [x] `VK_STENCIL_FACE_BACK_BIT`
  - [x] `VK_STENCILFRONT_AND_BACK`
- [x] `compareMask` of valid value
- [x] `wirteMask` of valid value
