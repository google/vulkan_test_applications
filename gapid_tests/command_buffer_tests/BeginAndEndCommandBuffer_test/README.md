# vkBeginCommandBuffer / vkEndCommandBuffer

## Signatures
```c++
VkResult vkBeginCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    const VkCommandBufferBeginInfo*             pBeginInfo);

VkResult vkEndCommandBuffer(
    VkCommandBuffer                  commandBuffer);
```

According to the Vulkan spec:
- vkBeginCommandBuffer
  - `commandBuffer` **must** not be in the recording state
  - `commandBuffer` **must** not currently be pending execution
  - `pBeginInfo` **must** be a pointer to a valid `VkCommandBufferBeginInfo`
    structure
  - If `commandBuffer` was allocated from a `VkCommandPool` which did not
    have the `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` flag set,
    commandBuffer **must** be in the initial state
  - If `commandBuffer` is a secondary command buffer, the `pInheritanceInfo`
    member of `pBeginInfo` **must** be a valid
    `VkCommandBufferInheritanceInfo` structure
  - If `commandBuffer` is a secondary command buffer and either the
    `occlusionQueryEnable` member of the `pInheritanceInfo` member of
    `pBeginInfo` is `VK_FALSE`, or the precise occlusion queries feature is
    not enabled, the `queryFlags` member of the `pInheritanceInfo` member
    `pBeginInfo` **must** not contain `VK_QUERY_CONTROL_PRECISE_BIT`
- vkEndCommandBuffer
  - `commandBuffer` must be a valid `VkCommandBuffer` in recording state.

## VkCommandBufferBeginInfo
```c++
typedef struct VkCommandBufferBeginInfo{
    VkStructureType                             sType;
    const void*                                 pNext;
    VkCommandBufferUsageFlags                   flags;
    const VkCommandBufferInheritanceInfo*       pInheritanceInfo;
} VkInstanceCreateInfo;
```

According to the Vulkan spec:
- VkCommandBufferBeginInfo
  - `sType` **must** be `VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO`
  - `pNext` **must** be `NULL`
  - `flags` is a bitmask which **must** be a valid combination of
    `VkCommandBufferUsageFlagBits` values
    - If `flags` contains `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`,
      the `renderPass` member of `pInheritanceInfo` **must** be a valid
      `VkRenderPass`, the `subpass` member of `pInheritanceInfo` **must** be
      a valid subpass index within the `renderPass` member of
      `pInheritanceInfo` and the `framebuffer` member of `pInheritanceInfo`
      must be either `VK_NULL_HANDLE` or a valid `VkFramebuffer` that is
      compatible with the `renderPass` member of `pInheritanceInfo`

## VkCommandBufferInheritanceInfo
```c++
typedef struct VkCommandBufferInheritanceInfo {
    VkStructureType                  sType;
    const void*                      pNext;
    VkRenderPass                     renderPass;
    uint32_t                         subpass;
    VkFramebuffer                    framebuffer;
    VkBool32                         occlusionQueryEnable;
    VkQueryControlFlags              queryFlags;
    VkQueryPipelineStatisticFlags    pipelineStatistics;
} VkCommandBufferInheritanceInfo;
```

According to the Vulkan spec:
- VkCommandBufferInheritanceInfo
  - `sType` **must** be `VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO`
  - `pNext` **must** be `NULL`
  - Valid handles of `framebuffer` and `renderPass` **must** have been created,
    allocated or retrieved from the same `VkDevice`
  - If the `inherited queries` feature is not enabled, `occlusionQueryEnable`
    **must** be `VK_FALSE`
  - If the `inherited queries` feature is enabled, `queryFlags` **must** be a
    valid combination of `VKQueryControlFlagBits` values
  - If the `pipeline statistics queries` feature is not enabled,
    `pipelineStatistics` **must** be 0

These tests should test the following cases:
- [x] `pInheritanceInfo` Null or not
- [ ] `flags` 0 or valid combinations
- [ ] `renderPass` `VK_NULL_HANDLE` or valid handle
- [ ] `framebuffer` `VK_NULL_HANDLE` or valid handle
- [ ] `subpass` 0 or valid number
- [ ] `occlusionQueryEnable` true or false
- [x] `vkEndCommandBuffer()` with a valid `VkCommandBuffer` in recording state.
