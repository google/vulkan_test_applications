# vkCmdExecuteCommands

## Signatures
```c++
void vkCmdExecuteCommands(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers);
```

According to the Vulkan spec:
- `commandBuffer` is a handle to a **primary** command buffer that the secondary
  command buffers are executed in, it **must** be allocated with a `level` of
  `VK_COMMAND_BUFFER_LEVEL_PRIMARY`
- Any given element of `pCommandBuffers` **must** have been allocated with a
  level of `VK_COMMAND_BUFFER_LEVEL_SECONDARY`, and **must not** be already
  pending execution in `commandBuffer`, or appear twice in `pCommandBuffers`,
  unless it was recorded with the `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`
  flag
- Any given element of `pCommandBuffers` **must not** be in the pending
  execution in any other `VkCommandBuffer`, unless it was recorded with the
  `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`
- Any given element of `pCommandBuffers` **must** be in the executable state
- Any given element of `pCommandBuffers` **must** have been allocated from a
  `VkCommandPool` that was created for the same queue family as the
  `VkCommandPool` from which `commandBuffer` was allocated
- If `vkCmdExecuteCommands` is being called within a render pass instance, any
  given element of `pCommandBuffers` **must** have been recorded with the
  `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`
- If `vkCmdExecuteCommands` is being called within a render pass instance, any
  given element of `pCommandBuffers` **must** have been recorded with
  `VkCommandBufferInheritanceInfo::subpass` set to the index of the subpass
  which the given command buffer will be executed in
- If `vkCmdExecuteCommands` is being called within a render pass instance, any
  given element of `pCommandBuffers` **must** have been recorded with a render
  pass that is compatible with the current render pass
- If `vkCmdExecuteCommands` is being called within a render pass instance, and
  any given element of `pCommandBuffers` was recorded with
  `VkCommandBufferInheritanceInfo::framebuffer` not equal to `VK_NULL_HANDLE`,
  that `VkFramebuffer` **must** match the `VkFramebuffer` used in the current
  render pass instance
- If `vkCmdExecuteCommands` is not being called within a render pass instance,
  any given element of `pCommandBuffers` **must not** have been recorded with the
  `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`
- If the *inherited queries* feature is not enabled, `commandBuffer` **must**
  not have any queries *active*
- If `commandBuffer` has a `VK_QUERY_TYPE_OCCLUSION` query active, then each
  element of `pCommandBuffers` **must** have been recorded with
  `VkCommandBufferInheritanceInfo::occlusionQueryEnable` set to `VK_TRUE`
- If `commandBuffer` has a `VK_QUERY_TYPE_OCCLUSION` query active, then each
  element of `pCommandBuffers` **must** have been recorded with
  `VkCommandBufferInheritanceInfo::queryFlags` having all bits set that are set
  for the query
- If `commandBuffer` has a `VK_QUERY_TYPE_PIPELINE_STATISTICS` query active,
  then each element of `pCommandBuffers` **must** have been recorded with
  `VkCommandBufferInheritanceInfo::pipelineStatistics` having all bits set that
  are set in the `VkQueryPool` the query uses
- Any given element of `pCommandBuffers` **must not** begin any query types that
  are active in `commandBuffer`
- The `VkCommandPool` that `commandBuffer` was allocated from **must** support
  transfer, graphics, or compute operations
- `commandBufferCount` **must** be greater than 0
- Both of `commandBuffer`, and the elements of `pCommandBuffers` **must** have
  been created, allocated, or retrieved from the same `VkDevice`

These tests should test the following cases:
- [x] `pCommandBuffers` without `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`
- [ ] `pCommandBuffers` with `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`,
  i.e., used in multiple `VkCommandBuffer`
- [ ] `vkCmdExecuteCommands` called within a render pass, i.e., created with
  `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`
- [x] `vkCmdExecuteCommands` called out of a render pass
- [x] `commandBuffer` with `VK_QUERY_TYPE_OCCLUSION` query inactive
- [ ] `commandBuffer` with `VK_QUERY_TYPE_OCCLUSION` query active, i.e., each
  element of `pCommandBuffers` **must** have been recorded with
  `VkCommandBufferInheritanceInfo::queryFlags` having all bits set that are set
  for the query
- [ ] `commandBuffer` with `VK_QUERY_TYPE_PIPELINE_STATISTICS` query inactive
- [ ] `commandBuffer` with `VK_QUERY_TYPE_PIPELINE_STATISTICS` query active,
  i.e., each element of `pCommandBuffers` **must** have been recorded with
  `VKCommandBufferInheritanceInfo::pipelineStatistics` having all bits set that
  are set in the `VkQueryPool` the query uses
- [x] `commandBuffer` and all elements of `pCommandBuffers` are allocated from
  the same `VkCommandPool`
- [ ] `commandBuffer` and elements of `pCommandBuffers` are allocated from
  the different `VkCommandPool`s, but the pools are created from the same queue
  family and `VkDevice`
