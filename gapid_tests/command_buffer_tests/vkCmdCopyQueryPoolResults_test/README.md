# vkCmdCopyQueryPoolResults

## Signatures
```c++
void vkCmdCopyQueryPoolResults(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags);
```

According to the Vulkan spec:
- `vkCmdCopyQueryPoolResults` is guaranteed to see the effect of previous uses
  of `vkCmdResetQueryPool` in the same queue, without any additional
  synchronization. Thus, the results will always reflect the most recent use of
  the query
- `commandBuffer` **must** be a valid `VkCommandBuffer` handle
- `firstQuery` **must** be less than the number of queries in `queryPool`
- The sum of `firstQuery` and `queryCount` **must** be less than or equal to
  the number of queries in `queryPool`
- `vkCmdCopyQueryPoolResults` **must** only be called outside of a render pass
  instance
- `dstOffset` **must** be less than the size of `dstBuffer`
- `dstBuffer` **must** have been created with `VK_BUFFER_USAGE_TRANSFER_DST_BIT`
  useage flag
- The `VkCommandPool` that `commandBuffer` was allocated from **must** support
  graphics, or compute operations
- If no bits are set in `flags`, results for all requested queries in the
  available state are written as 32-bit unsigned integer values, and nothing is
  written for queries in the unavailable state
- If `VK_QUERY_RESULT_64_BIT` is not set in flags then dstOffset and stride
  must be multiples of 4
- If `VK_QUERY_RESULT_64_BIT` is set in flags then dstOffset and stride must be
  multiples of 8
- If `VK_QUERY_RESULT_WAIT_BIT` is set, the implementation will wait for each
  query’s status to be in the available state before retrieving the numerical
  results for that query. This is guaranteed to reflect the most recent use of
  the query on the same queue, assuming that the query is not being
  simultaneously used by other queues. If the query does not become available
  in a finite amount of time (e.g. due to not issuing a query since the last
  reset), a `VK_ERROR_DEVICE_LOST` error may occur
- If `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` is set and
  `VK_QUERY_RESULT_WAIT_BIT` is not set, the availability is guaranteed to
  reflect the most recent use of the query on the same queue, assuming that the
  query is not being simultaneously used by other queues. As with
  `vkGetQueryPoolResults`, implementations must guarantee that if they return a
  non-zero availability value, then the numerical results are valid.
- If `VK_QUERY_RESULT_PARTIAL_BIT` is set, `VK_QUERY_RESULT_WAIT_BIT` is not
  set, and the query’s status is unavailable, an intermediate result value
  between zero and the final result value is written for that query.
- `VK_QUERY_RESULT_PARTIAL_BIT` **must NOT** be used if the pool’s queryType is
  `VK_QUERY_TYPE_TIMESTAMP`.

These tests should test the following cases:
- [x] `firstQuery` of value 0
- [x] `firstQuery` of value other than 0
- [x] `queryCount` of value 1
- [x] `queryCount` of value other than 1
- [x] `stride` of value 4
- [x] `stride` of value 8
- [x] `stride` of value 12
- [x] `dstOffset` of value 0
- [x] `dstOffset` of value other than 0
- [x] `flags` with no bit set
- [x] `flags` with `VK_QUERY_RESULT_64_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_WAIT_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_AVAILABILITY_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_PARTIAL_BIT` set
