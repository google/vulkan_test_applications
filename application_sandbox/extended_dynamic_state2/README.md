# extended_dynamic_state2

This sample renders 2 cubes which overlap with each other. Overlapping usually leads to z-fighting.
To prevent it, the depth bias is toggled between the draw calls for each cube with 'vkCmdSetDepthBiasEnableEXT'