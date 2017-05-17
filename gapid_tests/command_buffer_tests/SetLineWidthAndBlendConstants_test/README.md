# vkCmdSetLineWidth / vkCmdSetBlendConstants

## Signatures
```c++
void vkCmdSetLineWidth(
    VkCommandBuffer                             commandBuffer,
    float                                       lineWidth);

void vkCmdSetBlendConstants(
    VkCommandBuffer                             commandBuffer,
    const float                                 blendConstants[4]);
```


According to the Vulkan spec:
- `commandBuffer` **must** be a valid `VkCommandBuffer` handle
- `commandBuffer` **must** be in the *recording state*
- vkCmdSetLineWidth
  - The current bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_LINE_WIDTH` dynamic state enabled
  - If the *wide lines* feature is not enabled, `lineWidth` **must** be `1.0`
- vkCmdSetBlendConstants
  - The current bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_BLEND_CONSTANTS` dynamic state enabled

These tests should test the following cases:
- [x] `lineWidth` of valid value
- [x] `blendConstants` of valid values
