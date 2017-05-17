# vkGetQueryPoolResults

## Signatures
```c++
VkResult vkGetQueryPoolResults(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags);
```

# VkQueryResultFlagBits
```c++
typedef enum VkQueryResultFlagBits {
    VK_QUERY_RESULT_64_BIT = 0x00000001,
    VK_QUERY_RESULT_WAIT_BIT = 0x00000002,
    VK_QUERY_RESULT_WITH_AVAILABILITY_BIT = 0x00000004,
    VK_QUERY_RESULT_PARTIAL_BIT = 0x00000008,
} VkQueryResultFlagBits;
```

According to the Vulkan spec:
- The first query’s result is written starting at the first byte requested by
  the command, and each subsequent query’s result begins stride bytes later.
- Each query’s result is a tightly packed array of unsigned integers, either
  32- or 64-bits as requested by the command, storing the numerical results
  and, if requested, the availability status.
- If `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` is used, the final element of each
  query’s result is an integer indicating whether the query’s result is
  available, with any non-zero value indicating that it is available.
- Occlusion queries write one integer value - the number of samples passed.
  Pipeline statistics queries write one integer value for each bit that is
  enabled in the pipelineStatistics when the pool is created, and the
  statistics values are written in bit order starting from the least
  significant bit. Timestamps write one integer value.
- If more than one query is retrieved and `stride` is not at least as large as
  the size of the array of integers corresponding to a single query, the values
  written to memory are undefined.
- `firstQuery` **must** be less than the number of queries in `queryPool`
- If `VK_QUERY_RESULT_64_BIT` is not set in `flags` then `pData` and `stride`
  **must** be multiples of 4
- If `VK_QUERY_RESULT_64_BIT` is set in `flags` then `pData` and `stride`
  **must** be multiples of 8
- The sum of `firstQuery` and `queryCount` must be less than or equal to the
  number of queries in `queryPool`
- `dataSize` **must** be large enough to contain the result of each query
- If the `queryType` used to create `queryPool` was `VK_QUERY_TYPE_TIMESTAMP`,
  `flags` **must not** contain `VK_QUERY_RESULT_PARTIAL_BIT`
- If no bits are set in `flags`, and all requested queries are in the available
  state, results are written as an array of 32-bit unsigned integer values.
- If `VK_QUERY_RESULT_64_BIT` is not set and the result overflows a 32-bit
  value, the value may either wrap or saturate. Similarly, if
  `VK_QUERY_RESULT_64_BIT` is set and the result overflows a 64-bit value, the
  value may either wrap or saturate.
- If `VK_QUERY_RESULT_WAIT_BIT` is set, Vulkan will wait for each query to be
  in the available state before retrieving the numerical results for that query.
  In this case, `vkGetQueryPoolResults` is guaranteed to succeed and return
  `VK_SUCCESS` if the queries become available in a finite time (i.e. if they
  have been issued and not reset). If queries will never finish (e.g. due to
  being reset but not issued), then `vkGetQueryPoolResults` may not return in
  finite time.
- If `VK_QUERY_RESULT_WAIT_BIT` and `VK_QUERY_RESULT_PARTIAL_BIT` are both not
  set then no result values are written to `pData` for queries that are in the
  unavailable state at the time of the call, and `vkGetQueryPoolResults`
  returns `VK_NOT_READY`. However, availability state is still written to
  `pData` for those queries if `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` is set.
- If `VK_QUERY_RESULT_PARTIAL_BIT` is set, `VK_QUERY_RESULT_WAIT_BIT` is not
  set, and the query’s status is unavailable, an intermediate result value
  between zero and the final result value is written to `pData` for that query.
- `VK_QUERY_RESULT_PARTIAL_BIT` **must not** be used if the pool’s queryType is
  `VK_QUERY_TYPE_TIMESTAMP`.
- If `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` is set, the final integer value
  written for each query is non-zero if the query’s status was available or
  zero if the status was unavailable. When
  `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` is used, implementations must
  guarantee that if they return a non-zero availability value then the
  numerical results must be valid, assuming the results are not reset by a
  subsequent command.

These tests should test the following cases:
- [x] `firstQuery` of value 0
- [x] `firstQuery` of value other than 0
- [x] `queryCount` of value 1
- [x] `queryCount` of value other than 1
- [x] `stride` of value 4
- [x] `stride` of value 8
- [x] `stride` of value 12
- [x] `flags` with no bit set
- [x] `flags` with `VK_QUERY_RESULT_64_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_WAIT_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_AVAILABILITY_BIT` set
- [x] `flags` with `VK_QUERY_RESULT_PARTIAL_BIT` set
