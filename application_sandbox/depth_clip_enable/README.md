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

The near plane is placed at a distance of z=0.5 and the cube is placed
at z=1.5 so that the front part of the cube will should be pass through
the near plane. The resulting image should be a red cube at the back, a
small ring of green and then the blue in front. The red cube shows up
because it is in front except where it is clipped by the near plane. The
green cube shows up only in the small range where it is front of the
blue cube up to the point of being clamped to the near plane. The remainder
of the cube is blue as its depth values are neither clipped or clamped.

NOTE: This sample uses a floating point depth buffer and is required to
get the desired behaviour noted above.
