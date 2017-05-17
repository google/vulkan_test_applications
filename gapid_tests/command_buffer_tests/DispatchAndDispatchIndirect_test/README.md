# vkCmdDispatch / vkCmdDispatchIndirect

## Signatures
```c++
void vkCmdDispatch(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    groupCountX,
    uint32_t                                    groupCountY,
    uint32_t                                    groupCountZ);

void vkCmdDispatchIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset);
```

According to the Vulkan spec:
- `commandBuffer` **must** be in the recording state
- This command **must** only be called out of a render pass instance
- A valid compute pipeline must be bound to the current command buffer with
  `VK_PIPELINE_BIND_POINT_COMPUTE`
- Descriptors in each bound descriptor set, specified via
  `vkCmdBindDescriptorSets`, must be valid if they are statically used by the
  currently bound VkPipeline object, specified via `vkCmdBindPipeline`
- For each push constant that is statically used by the `VkPipeline` currently
  bound to `VK_PIPELINE_BIND_POINT_COMPUTE`, a push constant value must have
  been set for `VK_PIPELINE_BIND_POINT_COMPUTE`, with a `VkPipelineLayout` that
  is compatible for push constants with the one used to create the current
  `VkPipeline`
- If any `VkSampler` object that is accessed from a shader by the `VkPipeline`
  currently bound to `VK_PIPELINE_BIND_POINT_COMPUTE` uses unnormalized
  coordinates, it **must** NOT be used to sample from any `VkImage` with a
  `VkImageView` of the type `VK_IMAGE_VIEW_TYPE_3D`, `VK_IMAGE_VIEW_TYPE_CUBE`,
  `VK_IMAGE_VIEW_TYPE_1D_ARRAY`, `VK_IMAGE_VIEW_TYPE_2D_ARRAY` or
  `VK_IMAGE_VIEW_TYPE_CUBE_ARRAY`, in any shader stage, and it **must** NOT be
  used with any of the SPIR-V `OpImageSample*` or `OpImageSparseSample*`
  instructions with `ImplicitLod`, `Dref` or `Proj` in their name, or that
  includes a LOD bias or any offset values, in any shader stage
- If the *robust buffer* access feature is not enabled, and any shader stage in
  the `VkPipeline` object currently bound to `VK_PIPELINE_BIND_POINT_COMPUTE`
  accesses a uniform buffer, it must not access values outside of the range of
  that buffer specified in the currently bound descriptor set
- Any `VkImageView` being sampled with `VK_FILTER_LINEAR` as a result of this
  command must be of a format which supports linear filtering, as specified by
  the `VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT` flag in
  `VkFormatProperties`::linearTilingFeatures (for a linear image) or
  `VkFormatProperties`::optimalTilingFeatures(for an optimally tiled image)
  returned by `vkGetPhysicalDeviceFormatProperties`
- vkCmdDispatch
  - `groupCountX` **must** be less than or equal to
  `VkPhysicalDeviceLimits::maxComputeWorkGroupCount[0]`
  - `groupCountY` **must** be less than or equal to
  `VkPhysicalDeviceLimits::maxComputeWorkGroupCount[1]`
  - `groupCountZ` **must** be less than or equal to
  `VkPhysicalDeviceLimits::maxComputeWorkGroupCount[2]`
- vkCmdDispatchIndirect
  - If `buffer` is non-sparse then it **must** be bound completely and
  contiguously to a single `VkDeviceMemory` object
  - `buffer` **must** have been created with the
  `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT` bit set
  - `offset` **must** be a multiple of 4
  - The sum of `offset` and the size of `VkDispatchIndirectCommand` must be
  less than or equal to the size of `buffer`

These tests should test the following cases:
- [x] `groupCountX` of value larger than 1
- [ ] `groupCountY` of value larger than 1
- [ ] `groupCountZ` of value larger than 1
- [x] `buffer` of a valid `VkBuffer` for `vkCmdDispatchIndirect`
- [x] `offset` of value 0
- [ ] `offset` of value other than 0
