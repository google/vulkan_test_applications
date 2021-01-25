# Overlapping Frames

This sample renders a rotating triangle using overlapping frames, i.e. at each
main loop iteration, parts of several frames' rendering are being submitted.

This technique is used by some engines to pipeline frame rendering work. For
instance, a given rendering loop iteration may submit work for: the gbuffer
rendering of frame N+1, and the post-processing rendering and presentation of
frame N.

See the top-level comment in the source code for details about this sample
structure, and the associated Vulkan synchronization.
