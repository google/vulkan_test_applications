# vkCmdPushConstants

## Signatures
```c++
void vkCmdPushConstants(
    VkCommandBuffer                             commandBuffer,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues);
```

According to the Vulkan spec:
- `commandBuffer` **must** be a valid `VkCommandBuffer` handle in recording
  state
- `stageFlags` **must** be valid combination of `VkShaderStageFlagBits` values
  and **must not** be 0
- `pValues` **must** be a pointer to an array of `size` bytes
- `size` **must** be greater than 0 and **must** be a multiple of 4
- `offset` **must** be a multiple of 4 and **must** be less than
  `VkPhysicalDeviceLimits::maxPushConstantsSize`
- `stageFlags` **must** match exactly the shader stages used in `layout` for the
  range specified by `offset` and `size`
- Both of `commandBuffer`, and `layout` **must** have been created, allocated
  or retrieved from the same `VkDevice`

These tests should test the following cases:
- [x] `offset` of value 0
- [ ] `offset` of value non-zero
- [x] `stageFlags` of value `VK_SHADER_STAGE_VERTEX_BIT`
- [ ] `stageFlags` of value `VK_SHADER_STAGE_ALL`
