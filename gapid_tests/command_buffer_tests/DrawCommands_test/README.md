# vkCmdDraw / vkCmdDrawIndexed / vkCmdDrawIndirect / vkCmdDrawIndexedIndirect

## Signatures
```c++
void vkCmdDraw(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance);

void vkCmdDrawIndexed(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance);

void vkCmdDrawIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride);

void vkCmdDrawIndexedIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride);
```

# VkDrawIndirectCommand / VkDrawIndexedIndirectCommand
```c++
typedef struct VkDrawIndirectCommand {
    uint32_t    vertexCount;
    uint32_t    instanceCount;
    uint32_t    firstVertex;
    uint32_t    firstInstance;
} VkDrawIndirectCommand;

typedef struct VkDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} VkDrawIndexedIndirectCommand;
```

According to the Vulkan spec:
- A valid graphics pipeline **must** be bound to the current command buffer
  with `VK_PIPELINE_BIND_POINT_GRAPHICS`
- `commandBuffer` **must** be in the recording state
- These commands **must** only be called inside of a render pass instance
- vkCmdDrawIndexed
  - `(indexSize * (firstIndex + indexCount) + offset)` **must** be less than or
  equal to the size of the currently bound index buffer, with `indexSize` being
  based on the type specified by `indexType`, where the index buffer,
  `indexType`, and `offset` are specified via `vkCmdBindIndexBuffer`
- vkCmdDrawIndirect / vkCmdDrawIndexedIndirect
  - `offset` must be a multiple of 4
  - If `drawCount` is greater than 1, `stride` must be a multiple of 4 and must
  be greater than or equal to `sizeof(VkDrawIndirectCommand|VkDrawIndexedIndirectCommand)`
  - If `drawCount` is equal to 1,
  `(offset + sizeof(VkDrawIndirectCommand|VkDrawIndexedIndirectCommand))`
  **must** be less than or equal to the size of buffer
  - If `drawCount` is greater than 1, `(stride × (drawCount - 1) + offset +
  sizeof(VkDrawIndirectCommand|VkDrawIndexedIndirectCommand))` **must** be less
  than or equal to the size of buffer
  - If the `drawIndirectFirstInstance` feature is not enabled, all the
  `firstInstance` members of the
  `VkDrawIndirectCommand|VkDrawIndexedIndirectCommand` structures accessed by
  this command must be 0
  - `drawCount` **must** be less than or equal to
  VkPhysicalDeviceLimits::maxDrawIndirectCount

These tests should test the following cases:
- vkCmdDraw
  - [ ] `vertexCount` == 0
  - [x] `vertexCount` > 0
  - [ ] `instanceCount` == 0
  - [x] `instanceCount` > 0
  - [x] `firstVertex` == 0
  - [ ] `firstVertex` > 0
  - [x] `firstInstance` == 0
  - [ ] `firstInstance` > 0
- vkCmdDrawIndexed
  - [ ] `indexCount` == 0
  - [x] `indexCount` > 0
  - [ ] `instanceCount` == 0
  - [x] `instanceCount` > 0
  - [x] `firstIndex` == 0
  - [ ] `firstIndex` > 0
  - [x] `vertexOffset` == 0
  - [ ] `vertexOffset` > 0
  - [x] `firstInstance` == 0
  - [ ] `firstInstance` > 0
- vkCmdDrawIndirect
  - [x] `buffer` of valid indirect command buffer
  - [x] `offset` == 0
  - [ ] `offset` > 0
  - [ ] `drawCount` == 0
  - [x] `drawCount` > 0
  - [x] `stride` == 0
  - [ ] `stride` > 0
- vkCmdDrawIndexedIndirect
  - [x] `buffer` of valid indirect command buffer
  - [x] `offset` == 0
  - [ ] `offset` > 0
  - [ ] `drawCount` == 0
  - [x] `drawCount` > 0
  - [x] `stride` == 0
  - [ ] `stride` > 0
