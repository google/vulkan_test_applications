# Copy QueryPool Results (Host Reset)

This sample renders a wireframe torus that is shaded based on the
number of samples that passed the occlusion test in the previous frame.

It is the same as `copy_querypool_results`, except that the Query Pool is reset 
on the host side using the `VK_EXT_host_query_reset` device extension.
