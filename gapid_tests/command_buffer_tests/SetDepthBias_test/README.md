# vkCmdSetDepthBias

## Signatures
```c++
void vkCmdSetDepthBias(
    VkCommandBuffer                             commandBuffer,
    float                                       depthBiasConstantFactor,
    float                                       depthBiasClamp,
    float                                       depthBiasSlopeFactor);
```

According to the Vulkan spec:
- The currently bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_DEPTH_BIAS` dynamic state enabled
- If the depth bias clamping feature is not enabled, `depthBiasClamp` **must**
  be 0.0
- If `depthBiasEnable` in the currently bound graphics pipeline's
  `VkPipelineRasterizationStateCrateInfo` is `VK_FALSE`, no depth bias is
  applied and the fragment's depth values are unchanged.

These tests should test the following cases:
- [x] `depthBiasConstantFactor`, `depthBiasClamp` and `depthBiasSlopeFactor`
  of float values.
