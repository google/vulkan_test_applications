# depth_Stencil_resolve

This sample renders a rotating cube on a black floor with the reflection of the
cube shown on the floor. The reflection cube is confined within the floor by
using stencil buffer. The stencil buffer is multisampled at the default
multisample count and automatically resolved with a depth/stencil resolve in a
VkRenderPass2 using the max resolve mode.
