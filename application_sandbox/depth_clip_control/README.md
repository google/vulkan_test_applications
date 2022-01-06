# VK_EXT_depth_clip_control

This sample demonstrates the VK_EXT_depth_clip_control extension. This extension allows
the clipping of depth values to be changed from the default range of [0, 1] to the OpenGL
depth range of [-1, 1]. The sample renders two rotating cubes that would nominally be clipped
by the near plane. The top cube uses the [0, 1] depth range and is clipped as expected. The
second cube uses the [-1, 1] depth range and is not clipped. Additionally, the left side of the
rendered image shows the depth buffer contents and demonstrates the remapping of depth values due
to the NDC differences between the two cubes.
