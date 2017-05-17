# vkCmdSetViewport / vkCmdSetScissor / vkCmdBindPipeline

## Signatures
```c++
void vkCmdSetViewport(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstViewport,
    uint32_t                                    viewportCount,
    const VkViewport*                           pViewports);

void vkCmdSetScissor(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstScissor,
    uint32_t                                    scissorCount,
    const VkRect2D*                             pScissors);

void vkCmdBindPipeline(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipeline                                  pipeline);
```


According to the Vulkan spec:
- `pViewports` **must** point to `viewportCount` `VkViewport` structures
- `firstViewport` **must** be < `VkPhysicalDeviceLimits::maxViewports`
- `firstViewport` + `viewportCount` **must** be <=
    `VkPhysicalDeviceLimits::maxViewports`
- `viewportCount` **must** be > 0
- `pScissors` **must** point to `scissorCount` `VkRect2D` structures
- `firstScissor` **must** be < `VkPhysicalDeviceLimits::maxViewports`
- `firstScissor` + `scissorCount` **must** be <=
    `VkPhysicalDeviceLimits::maxViewports`
- `scissorCount` **must** be > 0

These tests should test the following cases:
- [x] `firstViewport` == 0
- [x] `firstScissor` == 0
- [ ] `firstViewport` > 0
- [ ] `firstScissor` > 0
- [x] `viewportCount` == 1
- [x] `scissorCount` == 1
- [ ] `viewportCount` > 1
- [ ] `scissorCount` > 1
- [x] `pipelineBindPoint` == `VK_PIPELINE_BIND_POINT_GRAPHICS`
- [ ] `pipelineBindPoint` == `VK_PIPELINE_BIND_POINT_COMPUTE`