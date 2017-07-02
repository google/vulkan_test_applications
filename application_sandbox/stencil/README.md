# Stencil

This sample renders a rotating cube on a black floor with the reflection of the
cube shown on the floor. The reflection cube is confined within the floor by
using stencil buffer. The actual value in the stencil buffer and the value used
to compare with the stencil buffer in a stencil test is controlled by calls to
`vkCmdSetStencilReference`, `vkCmdSetStencilWriteMask` and
`vkCmdSetStencilCompareMask`.
