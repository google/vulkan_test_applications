# Tiny Compute

This sample runs a computer shader. It computes the pariwise sum of
each element in storage buffers A and B and places them in C.

The `tiny_compute_glsl` sample uses GLSL.
The `tiny_compute_cl` sample uses OpenLC C.
The `tiny_compute_spvasm` sample uses SPIR-V assembly.  The included
`add_numbers.spvasm` source is generated from the OpenCL C version
using clspv, but with the entry point renamed from `adder` to `asmadder`.
