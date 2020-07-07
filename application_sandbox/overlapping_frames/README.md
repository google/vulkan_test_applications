# Overlapping Frames

This sample renders a triangle with a post-processing effect split into multiple
interleaved render passes.

## Synchronization Pattern

We have two render passes:
- Geometry Render Pass
- Post Processing Render Pass

For the rest of the document, we'll refer to them as GRP and PPRP.

For a given frame number N, the sample is synchronized as follows:

... -> GRP\_{N} -> PPRP\_{N-1} -> GRP\_{N+1} -> PPRP\_{N} -> ...

We use three sets of VkSemaphores and one set of VkFences to accomplish this
synchronization. Each set has a size equal to the number of swapchain images
(triple-buffered at a minimum).

## Semaphores

- gRenderFinished: Signaled when the Geometry Render Pass is completed.
- imageAcquired: Signaled when a swapchain image is acquired for the Post Processing Render Pass.
- postRenderFinished: Signaled when the Post Processing Render Pass is completed.

## Fences

- renderingFence: Signaled when the Post Processing Render Pass is completed.

We use only one set of fences to synchronize both render passes. Since the two
render passes are sequential - PPRP\_{N} cannot run before GRP\_{N} has been
completed.
