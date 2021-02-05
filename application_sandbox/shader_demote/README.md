# shader_demote

This sample renders a rotating cube with a cut out circle in the middle
of each face of the cube. It uses VK_EXT_shader_demote_to_helper_invocation
Vulkan extension and "demote" keyword in the fragment shader to ignore
output to framebuffer for some invocations.
