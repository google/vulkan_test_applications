# Common Math

math_common contains types that can be used across both GLSL and
C++. There is an equivalent math_common in the shader library that defines
all of these same types.

This means that struct definitions etc, can be shared between
C++/GLSL so long as they use these types.