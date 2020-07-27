# Depth Clip Enable

This sample demonstrates the VK_EXT_depth_clip_enable extension. This
extensions allows the depth clip and depth clamp functionality to be
controlled independently. The sample renders 3 identical prisms with
different clip/clamp settings. The three prisms are setup as follows:

Red Prism - This prism is rendered with the stock Vulkan options, clipping
enable and clamping disable.

Green Prism - This prism is rendered with the alternative stock Vulkan option,
clipping disable and clamping enabled.

Blue Prism - This prism is rendered with clipping disabled and clamping disabled,
this is only possible with VK_EXT_depth_clip_enable.

The three prisms are slightly offset from one another using the depth bias
support to place the red prism in front, the green prism in the middle and
the blue prism in the back.

The near plane is placed at a distance of z=1.2 and the far plane at z~=3
and the prism is placed at z=3 so that both the front part of the prism
and the back part of the prism with pass through the near and far planes.
The resulting image should be a red prism in the middle, the green prism
in the back and the blue prism in the front.

The red prism is in the "front" but is being clipped by the near/far planes.

The blue prism is in the "back" but isn't being clipped or clamped so
shows in the front as its depth values are lower than those being
clipped and clamped.

The green prism is in the "middle" but only shows up in the back where
is isn't being clipped and because its being clamped is showing up
in front of the blue prism.
