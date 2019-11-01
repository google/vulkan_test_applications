# mixed_sample_count

This sample renders a rotating cube on a black floor with the reflection of the
cube shown on the floor. The reflection cube is confined within the floor by
using stencil buffer. The stencil buffer is multisampled at the default
multisample count to improve coverage and the color buffer is sampled once. The
mixed sample count is controlled by calling the functions `EnableMultisampling`
and `EnableMixedMultisampling`. The use of mixed sample count is enable by
requesting the `VK_AMD_mixed_attachment_samples`
