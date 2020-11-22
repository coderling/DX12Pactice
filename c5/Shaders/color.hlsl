cbuffer cbPerObject : register(b0)
{
    float4x4 g_worldviewproj;
    float gtime;
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
    vin.pos.xy += 0.5f * sin(vin.pos.x) * sin(3.0f * gtime);
    vin.pos.z *= 0.6f + 0.4 * sin(2.0f * gtime);

    vout.pos = mul(float4(vin.pos, 1.0f), g_worldviewproj);
    vout.color = vin.color;
    return vout;
}

float4 PS(VertexOut pin) :SV_Target
{
    clip(pin.color.r - 0.5f);
    return pin.color;
}