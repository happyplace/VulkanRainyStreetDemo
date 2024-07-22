struct VertexIn
{
[[vk::location(0)]] float3 posL : POSITION;
[[vk::location(1)]] float3 normalL : NORMAL;
[[vk::location(2)]] float2 texC : TEXCOORD;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
[[vk::location(0)]] float3 posW : POSITION;
[[vk::location(1)]] float3 normalW : NORMAL;
[[vk::location(2)]] float2 texC : TEXCOORD;
};

struct FrameBuffer
{
    float4x4 view_proj;
    float3 eye_pos;
};

cbuffer frame : register(b0, space0) { FrameBuffer frame; }

struct ObjectBuffer
{
    float4x4 world;
};

cbuffer object : register(b0, space1) { ObjectBuffer object; }

VertexOut vs(VertexIn vin)
{
    VertexOut vout;

    float4 posW = mul(float4(vin.posL, 1.0f), object.world);
    vout.posW = posW.xyz;

    // Transform to homogeneus clip space
    vout.posH = mul(posW, frame.view_proj);

    vout.normalW = mul(vin.normalL, (float3x3)object.world);

    vout.texC = vin.texC;

    return vout;
}

float4 fs(VertexOut vout) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
