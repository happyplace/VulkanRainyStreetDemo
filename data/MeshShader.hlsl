struct VertexIn
{
[[vk::location(0)]] float3 posL : POSITION0;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
};

// layout(location = 1) in vec3 in_normal;
// layout(location = 2) in vec2 in_uv;

//layout(location = 0) out vec3 out_normalW;
// layout(location = 1) out vec3 out_positionW;
// layout(location = 2) out vec2 out_uv;

// layout(std140, set = 0, binding = 0) uniform FrameBuffer
// {
//     mat4 view_proj;
//     vec3 eye_pos;
// } Frame;

struct ObjectBuffer
{
    float4x4 world_view_proj;
};

cbuffer object : register(b0 /*1*/, space0) { ObjectBuffer object; }

VertexOut vs(VertexIn vin)
{
    VertexOut vout;

    // Transform to homogeneus clip space
    vout.posH = mul(float4(vin.posL, 1.0f), object.world_view_proj);

    return vout;
}

float4 fs(VertexOut vout) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
