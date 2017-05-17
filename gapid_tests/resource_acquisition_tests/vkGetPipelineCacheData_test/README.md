# vkGetPipelineCacheData

## Signatures
```c++
VkResult vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData);
```

```c++
typedef enum VkPipelineCacheHeaderVersion {
    VK_PIPELINE_CACHE_HEADER_VERSION_ONE = 1,
} VkPipelineCacheHeaderVersion;
```

According to the Vulkan spec:
- `device` **must** be a valid VkDevice
- `pDataSize` is a pointer to a value related to the amount of data in the
  pipeline cache. It **must** be a pointer to a `size_t` value
- `pData` is either `NULL` or a pointer to a buffer
- If `pData` is `NULL`, then the maximum size of the data that can be retrieved
  from the pipeline cache, in bytes, is returned in `pDataSize`. Otherwise,
  `pDataSize` **must** point to a variable set by the user to the size of the
  buffer, in bytes, pointed to by `pData`, and on return the variable is
  overwritten with the amount of data actually written to `pData`
- If the value referenced by `pDataSize` is not 0, and `pData` is not `NULL`,
  `pData` **must** be a pointer to an array of `pDataSize` bytes
- If `pDataSize` is less than the maximum size that can be retrieved by the
  pipeline cache, at most `pDataSize` bytes will be written to `pData`, and
  `vkGetPipelineCacheData` will return `VK_INCOMPLETE`. Any data written to
  `pData` is valid and can be provided as the `pInitialData` member of the
  `VkPipelineCacheCreateInfo` structure passed to `vkCreatepipelineCache`
- The initial bytes written to `pData` **must** be a header consisting of the
  following members:
  - Length in bytes of the entire pipeline cache header written as a stream of
  bytes, with the least significant byte first
  - a `VkPipelineCacheHeaderVersion` value written as a stream of bytes, with
  the least significant byte first. The value must be one of the enum values
  of `VkPipelineCacheHeaderVersion`
  - a vendor ID euqal to `VkPhysicalDeviceProperties::vendorID` written as a
  stream of bytes, with the least significant byte first
  - a device ID equal to `VkPhysicalDeviceProperties::deviceID` written as a
  stream of bytes, with the least significant byte first
  - a pipeline cache ID equal to `VkPhysicalDeviceProperties::pipelineCacheUUID`

These tests should test the following cases:
- `pData` is `NULL`
  - [x] `pDataSize` refer to a value of 0
  - [ ] `pDataSize` refer to a value other than 0
- `pData` is not `NULL`
  - [x] `pDataSize` refer to a value of the size of the pipeline cache
  - [ ] `pDataSize` refer to a value less than the size of the pipeline cache
- [x] Creates a graphics pipeline with pipeline cache data
