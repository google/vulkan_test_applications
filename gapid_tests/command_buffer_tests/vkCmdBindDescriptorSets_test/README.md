# vkCmdBindDescriptorSets

## Signatures
```c++
void vkCmdBindDescriptorSets(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets);
```

According to the Vulkan spec:
- `pDescriptorSets` **must** be a pointer to an array of `descriptorSetCount`
  valid `VkDescriptorSet` handles
- If `dynamicOffsetCount` is not 0, pDynamicOffsets **must** be a pointer to
  an array of `dynamicOffsetCount` `uint32_t` values
- `commandBuffer` **must** be in the recording state
- The `VkCommandPool` that commandBuffer was allocated from **must** support
  graphics, or compute operations
- `descriptorSetCount` **must** be greater than 0

These tests should test the following cases:
- `pipelineBindPoint`
  - [x] `VK_PIPELINE_BIND_POINT_GRAPHICS`
  - [ ] `VK_PIPELINE_BIND_POINT_COMPUTE`
- `firstSet`
  - [x] == 0
  - [ ] != 0
- `descriptorSetCount`
  - [x] == 1
  - [ ] > 1
- `dynamicOffsetCount`
  - [x] == 0
  - [ ] != 0
- `pDynamicOffsets`
  - [x] == nullptr
  - [ ] != nullptr
