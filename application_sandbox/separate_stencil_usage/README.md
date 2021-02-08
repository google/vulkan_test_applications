# Separate Stencil Usage

This sample execerises the extension VK_EXT_separate_stencil_usage by creating
a depth/stencil image where only the stencil aspect can be used as an input
attachment. The sample first draws a cube and a plane, the pixels for the plane
are also written to the stencil buffer. A second pass uses a full screen quad
to draw the stencil buffer onto the right hand side of the image (creating a
sort of color/stencil side-by-side). This second pass uses the stencil buffer
as an input attachment to the fragment shader, which is made possible with the
separate stencil usages (the depth aspect only supports depth/stencil attachment
usage).
