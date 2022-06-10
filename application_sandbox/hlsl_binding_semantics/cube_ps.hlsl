struct PSInput
{
    [[vk::location(0)]]float4 color : COLOR;
};

// Texture2D tex : register(t0);
// SamplerState s : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    // return tex.Sample(s, float2(0,0));
    return input.color;
}