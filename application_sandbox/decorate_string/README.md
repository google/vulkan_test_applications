# Decorate String

This sample uses HLSL semantics and `OpDecorateString` to render a rotating cube
on the screen. The use of decorate string is enabled by requesting the device
extensions `VK_GOOGLE_decorate_string` and `VK_GOOGLE_hlsl_functionality1`.
Decorate string is used to set the user semantics `SV_POSITION` and `TEXCOORD`
in the hlsl shader.