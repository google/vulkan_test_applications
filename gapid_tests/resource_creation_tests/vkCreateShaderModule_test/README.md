# vkCreateShadermodule / vkDestroyShaderModule

## Signatures
```c++
VkResult vkCreateShaderModule(
    VkDevice                                    device,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkShaderModule*                             pShaderModule);

void vkDestroyShaderModule(
    VkDevice                                    device,
    VkShaderModule                              shaderModule,
    const VkAllocationCallbacks*                pAllocator);
```

# VkSwapchainCreateInfoKHR
```c++
typedef struct VkShaderModuleCreateInfo {
    VkStructureType              sType;
    const void*                  pNext;
    VkShaderModuleCreateFlags    flags;
    size_t                       codeSize;
    const uint32_t*              pCode;
} VkShaderModuleCreateInfo;
```


According to the Vulkan spec:
- `pCreateInfo` **must** be a pointer to a valid `VkShaderModuleCreateInfo`
structure
- `pNext` **must** be `nullptr`
- `flags` **must** be `0`
- `pCode` is a pointer to `codeSize/4` uint32_t values representing the spirv.

These tests should test the following cases:
- [x] Valid usage