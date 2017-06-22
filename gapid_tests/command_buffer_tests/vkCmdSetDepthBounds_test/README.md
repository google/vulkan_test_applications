# vkCmdSetDepthBounds

## Signatures
```c++
void vkCmdSetDepthBounds(
    VkCommandBuffer                             commandBuffer,
    float                                       minDepthBounds,
    float                                       maxDepthBounds)
```

According to the Vulkan spec:
- The currently bound graphics pipeline **must** have been created with the
  `VK_DYNAMIC_STATE_DEPTH_BOUNDS` dynamic state enabled
- `minDepthBounds` **must** be between 0.0 and 1.0, inclusive
- `maxDepthBounds` **must** be between 0.0 and 1.0, inclusive
- If there is no depth framebuffer or if the depth bounds test is disabled, it
  is as if the depth bounds test always passes
- `depthBounds` feature in `VkPhysicalDeviceFeatures` **must** be enabled. If
  the feature is not enabled, in `VkPipelineDepthStencilStateCreateInfo`
  structure, `depthBoundsTestEnable` **must** be set to `VK_FALSE`, and then
  `VK_DYNAMIC_STATE_DEPTH_BOUNDS` will not be a valid state to be added to the
  pipeline

These tests should test the following cases:
- [x] `minDepthBounds` and `maxDepthBounds` of float values.
