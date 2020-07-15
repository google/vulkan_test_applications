# Depth Clipe Enable

This sample demonstrate the VK_EXT_depth_clip_enable features. This feature
allows the depth clip and depth clamp functionality to be controlled
independently. The sample renders 3 identical cubes with different
clip/clamp settings. The three cubes are setup as follows:

Red Cube - This cube is render with the stock Vulkan options, clipping
enable and clamping disable.

Green Cube - This cube is render with the alternative stock Vulkan option,
clipping disable and clamping enabled.

Blue Cube - This cube is rendered with clipping disabled and clamping disabled,
this is only possible with VK_EXT_depth_clip_enable.

The three cubes are slightly offset from one another using the depth bias
support to place the red cube in front, the green cube in the middle and
the blue cube in the back.

The near plane is placed at a distance of z=1.2 and the far plane at z~=3
and the cube is placed at z=3 so that both the front part of the cube
and the back part of the cube with pass through the near and far planes.
The resulting image should be a red cube in the middle, the green cube
in the back and the blue cube in the front.

The red cube is in the "front" but is being clipped by the near/far planes.

The blue cube is in the "back" but isn't being clipped or clamped so
shows in the front as its depth values are lower than those being
clippined and clamped.

The green cube is in the "middle" but only shows up in the back where
is isn't being clipped and because its being clamped is showing up
in front of the blue cube.
