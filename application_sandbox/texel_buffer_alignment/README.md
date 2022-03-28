# VK_EXT_texel_buffer_alignment

This sample exercises the VK_EXT_texel_buffer_alignment extension by allocating two
types of texel buffers (one storage and one uniform) with the minimum required
offset alignment. This minimum is queried from the device properties structure
provided via this extension. The resulting buffers are then used in a shader
to render a textured cube. The texture is a rough 2x3 texture where the top 1x3
row is sourced from the uniform buffer and the bottom row from the storage
buffer. The buffers are dynamically updated each frame to demonstrate that data
can be written with the effective buffer offset alignment.
