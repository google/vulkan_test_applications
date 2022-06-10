struct CameraData {
    float4x4 projection;
    float4x4 transform;
};

ConstantBuffer<CameraData> CameraDataCB : register(b0);

struct VertexShaderInput
{
    [[vk::location(0)]]float3 Position : POSITION;
    [[vk::location(1)]]float2 UV : TEXCOORD;
    [[vk::location(2)]]float3 Normal : NORMAL;
};

struct VertexShaderOutput
{
    [[vk::location(0)]]float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput VSMain(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    matrix MVP = mul(CameraDataCB.projection, CameraDataCB.transform);
    OUT.Position = mul(MVP, float4(IN.Position, 1.0f));
    // OUT.Position = float4(IN.Position, 1.0f);
    OUT.Color = float4(IN.UV, 0.0f, 1.0f);

    return OUT;
}