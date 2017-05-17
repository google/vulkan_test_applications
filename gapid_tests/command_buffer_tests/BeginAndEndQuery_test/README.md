# vkCmdBeginQuery / vkCmdEndQuery

## Signatures
```c++
void vkCmdBeginQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query,
    VkQueryControlFlags                         flags);

void vkCmdEndQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query);
```

# VkQueryControlFlagBits
```c++
typedef enum VkQueryControlFlagBits {
    VK_QUERY_CONTROL_PRECISE_BIT = 0x00000001,
} VkQueryControlFlagBits;
```

According to the Vulkan spec:
- `vkCmdBeginQuery`
  - `query` **must** be less than the number of queries in `queryPool`
  - The query identified by `queryPool` and `query` **must** currently **not**
  be active
  - The query identified by `queryPool` and `query` **must** be unavailable
  - If the precise occlusion queries feature is not enabled, or the `queryType`
  used to create `queryPool` was not `VK_QUERY_TYPE_OCCLUSION`, `flags` **must
  not** contain `VK_QUERY_CONTROL_PRECISE_BIT`
  - `query` **must** have been created with a `queryType` that differs from
  that of any other queries that ahve been made *active*, and are currently
  still active within `commandBuffer`
  - If the `queryType` used to create `queryPool` was `VK_QUERY_TYPE_OCCLUSION`,
  the `VkCommandPool` that `commandBuffer` was allocated from **must** support
  graphics operations
  - If the `queryType` used to create `queryPool` was
  `VK_QUERY_TYPE_PIPELINE_STATISTICS` and any of the `pipelineStatistics`
  indicate graphics operations, the `VkCommandPool` that `commandBuffer` was
  allocated from **must** support graphics operations
  - If the `queryType` used to create `queryPool` was
  `VK_QUERY_TYPE_PIPELINE_STATISTICS` and any of the `pipelineStatistics`
  indicate compute operations, the `VkCommandPool` that `commandBuffer` was
  allocated from **must** support compute operations
- `vkCmdEndQuery`
  - `commandBuffer` **must** be in the recording state
  - `query` **must** be less than the number of queries in `queryPool`

These tests should test the following cases:
- [x] `queryPool` and `query` of valid values
- [x] `flags` with `VK_QUERY_CONTROL_PRECISE_BIT` set
