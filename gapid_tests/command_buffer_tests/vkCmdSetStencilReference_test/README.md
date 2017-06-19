# vkCmdSetStencilReference

## Signatures
```c++
void vkCmdSetStencilReference(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    reference);
```

According to the Vulkan spec:
- The currently bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_STENCIL_REFERENCE` dynamic state enabled.
- `faceMask` **must** be a valid combination of `VkStencilFaceFlagBits` values
- `faceMask` must not be 0
- `commandBuffer` **must** be in the recording state

These tests should test the following cases:
- `flagMask` of valid value:
  - [x] `VK_STENCIL_FACE_FRONT_BIT`
  - [x] `VK_STENCIL_FACE_BACK_BIT`
  - [x] `VK_STENCIL_FRONT_AND_BACK`
- [x] `reference` of valid values
