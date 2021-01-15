# extended_dynamic_state

This sample displays two rotating cubes on the right side of the screen with
a pink background.
It uses VK_EXT_external_dynamic_state extension to dynamically
(during command buffer recording) set the following state:

* bound vertex buffers specifying the strides and sizes using vkCmdBindVertexBuffers2EXT
* cull mode uisng vkCmdSetCullModeEXT
* whether depth bounds test is enabled using vkCmdSetDepthBoundsEnableEXT
* depth compare op using vkCmdSetDepthCompareOpEXT
* whether depth test is enabled using vkCmdSetDepthTestEnableEXT
* whether depth write is enabled using vkCmdSetDepthWriteEnableEXT
* front face using vkCmdSetFrontFaceEXT
* primitive topology using vkCmdSetPrimitiveTopologyEXT
* scissors using vkCmdSetScissorWithCountEXT
* stencil operation using vkCmdSetStencilOpEXT
* whether stencil test is enabled using vkCmdSetStencilTestEnableEXT
* viewports using vkCmdSetViewportWithCountEXT
