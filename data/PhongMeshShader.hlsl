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

struct Light
{
    float3 strength;
    float falloff_start; // point/spot light only
    float3 direction;   // directional/spot light only
    float falloff_end;   // point/spot light only
    float3 position;    // point light only
    float spot_power;    // spot light only
};

struct Material
{
    float4 diffuse_albedo;
    float3 fresnelR0;
    float shininess;
};

#define MaxLights 16

struct FrameBuffer
{
    float4x4 view_proj;
    float3 eye_pos;
    float padding0;
    float4 ambient_light;
    Light lights[MaxLights];
};

cbuffer frame : register(b0, space0) { FrameBuffer frame; }

struct ObjectBuffer
{
    float4x4 world;
    Material material;
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

float calc_attenuation(float d, float falloff_start, float falloff_end)
{
    // Linear falloff.
    return saturate((falloff_end-d) / (falloff_end - falloff_start));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 schlick_fresnel(float3 R0, float3 normal, float3 light_vec)
{
    float cos_incident_angle = saturate(dot(normal, light_vec));

    float f0 = 1.0f - cos_incident_angle;
    float3 reflect_percent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflect_percent;
}

float3 blinn_phong(float3 light_strength, float3 light_vec, float3 normal, float3 to_eye, Material mat)
{
    const float m = mat.shininess * 256.0f;
    float3 half_vec = normalize(to_eye + light_vec);

    float roughness_factor = (m + 8.0f)*pow(max(dot(half_vec, normal), 0.0f), m) / 8.0f;
    float3 fresnel_factor = schlick_fresnel(mat.fresnelR0, half_vec, light_vec);

    float3 spec_albedo = fresnel_factor * roughness_factor;

    // Our spec formula goes outside [0,1] range, but we are
    // doing LDR rendering.  So scale it down a bit.
    spec_albedo = spec_albedo / (spec_albedo + 1.0f);

    return (mat.diffuse_albedo.rgb + spec_albedo) * light_strength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 compute_directional_light(Light L, Material mat, float3 normal, float3 to_eye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 light_vec = -L.direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(light_vec, normal), 0.0f);
    float3 light_strength = L.strength * ndotl;

    return blinn_phong(light_strength, light_vec, normal, to_eye, mat);
}

float4 compute_lighting(Material mat,
                       float3 pos, float3 normal, float3 to_eye,
                       float3 shadow_factor)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);

    int i = 0;

#if (NUM_DIRECTIONAL_LIGHTS > 0)
    for(i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i)
    {
        result += shadow_factor[i] * compute_directional_light(frame.lights[i], mat, normal, to_eye);
    }
#endif

    return float4(result, 0.0f);
}

float4 fs(VertexOut fin) : SV_Target
{
    fin.normalW = normalize(fin.normalW);

    float3 to_eyeW = normalize(frame.eye_pos - fin.posW);

    float4 ambient = frame.ambient_light * object.material.diffuse_albedo;

    float3 shadow_factor = 1.0f;
    float4 direct_light = compute_lighting(object.material, fin.posW, fin.normalW, to_eyeW, shadow_factor);

    float4 lit_colour = ambient + direct_light;

    lit_colour.a = object.material.diffuse_albedo.a;

    return lit_colour;
}
