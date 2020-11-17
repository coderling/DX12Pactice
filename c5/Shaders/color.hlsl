cbuffer cbPerObject : register(b0)
{
    float4x4 g_worldviewproj;
};

struct VertexIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.pos = mul(float4(vin.pos, 1.0f), g_worldviewproj);
    vout.color = vin.color;
    return vout;
}

float4 PS(VertexOut pin) :SV_Target
{
    return pin.color;
}