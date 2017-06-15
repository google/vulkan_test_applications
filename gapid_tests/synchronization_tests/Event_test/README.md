# vkCreateEvent / vkDestroyEvent / vkGetEventStatus / vkSetEvent / vkResetEvent

## Signatures
```c++
VkResult vkCreateEvent(
    VkDevice                                    device,
    const VkEventCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkEvent*                                    pEvent);

void vkDestroyEvent(
    VkDevice                                    device,
    VkEvent                                     event,
    const VkAllocationCallbacks*                pAllocator);

VkResult vkGetEventStatus(
    VkDevice                                    device,
    VkEvent                                     event);

VkResult vkSetEvent(
    VkDevice                                    device,
    VkEvent                                     event);

VkResult vkResetEvent(
    VkDevice                                    device,
    VkEvent                                     event);
```

# VkEventCreateInfo
```c++
typedef struct VkEventCreateInfo {
    VkStructureType       sType;
    const void*           pNext;
    VkEventCreateFlags    flags;
} VkEventCreateInfo;

```

According to the Vulkan spec:
- VkEventCreateInfo
  - `flags` **must** be 0
- vkGetEventStatus
  - If a `vkCmdSetEvent` or `vkCmdResetEvent` command is in a command buffer
  that is in the *pending state*, then the value returned by this command may
  immediately be out of date.
- vkSetEvent
  - If `event` is already in the signaled state when `vkSetEvent` is executed,
  then `vkSetEvent` has no effect, and no event signal operation occurs.
- vkResetEvent
  - If `event` is already i the unsignaled state when `vkResetEvent` is
  executed, then `vkResetEvent` has no effect, and no event unsignal operation
  occurs.

These tests should test the following cases:
- [x] `vkCreateEvent` and `vkDestroyEvent`
- [x] `vkGetEventStatus`
- [x] `vkSetEvent` and `vkResetEvent`

# vkCmdSetEvent / vkCmdResetEvent / vkCmdWaitEvent

## Signatures
```c++
void vkCmdSetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask);

void vkCmdResetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask);

void vkCmdWaitEvents(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers);
```

# VkMemoryBarrier / VkBufferMemoryBarrier / VkImageMemoryBarrier
Struct definition in [vkCmdPipelineBarrier_test/README.md](../vkCmdPipelineBarrier_test/README.md)

According to the Vulkan spec:
- vkCmdSetEvent and vkCmdResetEvent
  - If the `tessellation shaders` feature is not enabled, `stageMask` **must**
  not contain `VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT` or
  `VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT`
  - If the `geometry shaders` feature is not enabled, `stageMask` **must** not
  contain `VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT`
  - When `vkCmdReset` command executes, `event` **must** NOT be waited on by a
  `vkCmdWaitEvents` commands that is currently executing
  - Those commands **must** be called outside of a renderpass
- vkCmdWaitEvents
  - If the `geometry shaders` feature is not enabled, `srcStageMask` and
  `dstStageMask` **must** not contain `VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT`
  - If the `tessellation shaders` feature is not enabled, `srcStageMask` and
  `dstStageMask` **must** not contain
  `VK_PIPEINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT` or
  `VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT`
  - If `pEvents` includes one or more events that will be signaled by
  `vkSetEvent` after `commandBuffer` has been submitted to a queue, then
  `vkCmdWaitEvents` **must** NOT be called inside a render pass instance
  - Any pipeline stage included in `srcStageMask` or `dstStageMask` must be
  supported by the capabilities of the queue family specified by the
  `queueFamilyIndex` member of the `VkCommandPoolCreateInfo` structure that was
  used to create the `VkCommandPool` that commandBuffer was allocated from, as
  specified in the table of supported pipeline stages.
  - Any given element of `pMemoryBarriers`, `pBufferMemoryBarriers` or
  `pImageMemoryBarriers` **must** NOT have any access flag included in its
  `srcAccessMask` member if that bit is not supported by any of the pipeline
  stages in `srcStageMask`, as specified in the table of supported access types
  - Any given element of `pMemoryBarriers`, `pBufferMemoryBarriers` or
  `pImageMemoryBarriers` **must** NOT have any access flag included in its
  `dstAccessMask` member if that bit is not supported by any of the pipeline
  stages in `dstStageMask`, as specified in the table of supported access types
  - `srcStageMask` and `dstStageMask` **must** NOT be 0
  - `eventCount` **must** be greater than 0

These tests should test the following cases:
- vkCmdSetEvent
  - [x] `event` and `stageMask` of valid values
- vkCmdRestEvent
  - [x] `event` and `stageMask` of valid values
- vkCmdWaitEvents
  - [x] `srcStageMask` and `dstStageMask` of valid values
  - [x] Non-zero `memoryBarrierCount` and non-null `pMemoryBarriers`
  - [x] Non-zero `bufferMemoryBarrierCount` and non-null `pBufferMemoryBarriers`
  - [x] Non-zero `imageMemoryBarrierCount` and non-null `pImageMemoryBarriers`
  - [x] Non-zero `eventCount` and non-null `pEvents`
