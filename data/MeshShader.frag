#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#ifndef MAX_SAMPLED_TEXTURE_COUNT
#define MAX_SAMPLED_TEXTURE_COUNT 1
#endif

struct DirectionalLightBuf
{
    vec3 uStrength;
    float padding0;
    vec3 uDirection;
};

struct SpotLightBuf
{
    vec3 uStrength;
    float uFalloffStart;
    vec3 uDirection;
    float uFalloffEnd;
    vec3 uPosition;
    float uSpotPower;
};

struct PointLightBuf
{
    vec3 uStrength;
    float uFalloffStart;
    vec3 uPosition;
    float uFalloffEnd;
};

layout(std140, set = 0, binding = 0) uniform FrameBuf
{
    mat4 uViewProj;
    vec3 uEyePosW;
    float padding0;
    vec4 uAmbientLight;
    DirectionalLightBuf uDirLight;
    SpotLightBuf uSpotLight;
    PointLightBuf uPointLights[2];
} Frame;

layout(set = 2, binding = 0) uniform sampler samplerColour;
layout(set = 3, binding = 0) uniform texture2D sampledTexture[MAX_SAMPLED_TEXTURE_COUNT];

layout(std140, set = 1, binding = 0) uniform ObjectBuf
{
    mat4 uWorld;
    vec4 uDiffuseAlbedo;
    vec3 uFresnelR0;
    float uRoughness;
    uint uTextureIndex;
} Object;

layout(location = 0) in vec3 vNormalW;
layout(location = 1) in vec3 vPositionW;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fFragColor;

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
vec3 SchlickFresnel(vec3 R0, vec3 normal, vec3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    vec3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

vec3 BlinnPhong(vec3 lightStrength, vec3 lightVec, vec3 normal, vec3 toEye)
{
    float shininess = 1.0f - Object.uRoughness;
    float m = shininess * 256.0f;
    vec3 halfVec = normalize(toEye + lightVec);

    float roughtnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    vec3 fresnelFactor = SchlickFresnel(Object.uFresnelR0, halfVec, lightVec);

    vec3 specAlbedo = fresnelFactor * roughtnessFactor;

    // Our spec formula goes outside [0,1] range, but we are
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (Object.uDiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

vec3 ComputeDirectionalLight(DirectionalLightBuf light, vec3 normal, vec3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    vec3 lightVec = -light.uDirection;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    vec3 lightStrength = light.uStrength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

vec3 ComputeSpotLight(SpotLightBuf light, vec3 pos, vec3 normal, vec3 toEye)
{
    // The vector from the surface to the light.
    vec3 lightVec = light.uPosition - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > light.uFalloffEnd)
        return vec3(0.0f);

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    vec3 lightStrength = light.uStrength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.uFalloffStart, light.uFalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, light.uDirection), 0.0f), light.uSpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

vec3 ComputePointLight(PointLightBuf light, vec3 pos, vec3 normal, vec3 toEye)
{
    // The vector from the surface to the light.
    vec3 lightVec = light.uPosition - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > light.uFalloffEnd)
        return vec3(0.0f);

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    vec3 lightStrength = light.uStrength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.uFalloffStart, light.uFalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

void main()
{
#ifdef IS_WIREFRAME
    fFragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
#else
    vec3 normalizedNormalW = normalize(vNormalW);
    vec3 toEyeW = normalize(Frame.uEyePosW - vPositionW);

#ifdef USE_TEXTURE
    vec2 uv = inUV;
#ifdef USE_TEXTURE_SAMPLE_SCALE
    uv *= 100.0f;
#endif // USE_TEXTURE_SAMPLE_SCALE
    vec3 rgb = (texture(sampler2D(sampledTexture[Object.uTextureIndex], samplerColour), uv) * Object.uDiffuseAlbedo).xyz;
#else
    vec3 rgb = Object.uDiffuseAlbedo.xyz;
#endif // USE_TEXTURE

    const float shadowFactor = 1.0f;

    vec3 lightResult = vec3(0.0f, 0.0f, 0.0f) + Frame.uAmbientLight.xyz;
#ifdef USE_DIRECTIONAL_LIGHT
    lightResult += shadowFactor * ComputeDirectionalLight(Frame.uDirLight, normalizedNormalW, toEyeW);
#endif // USE_DIRECTIONAL_LIGHT

#ifdef USE_SPOT_LIGHT
    lightResult += shadowFactor * ComputeSpotLight(Frame.uSpotLight, vPositionW, normalizedNormalW, toEyeW);
#endif // USE_SPOT_LIGHT

#ifdef USE_POINT_LIGHT
    lightResult += shadowFactor * ComputePointLight(Frame.uPointLights[0], vPositionW, normalizedNormalW, toEyeW);
    lightResult += shadowFactor * ComputePointLight(Frame.uPointLights[1], vPositionW, normalizedNormalW, toEyeW);
#endif // USE_POINT_LIGHT

    vec3 finalColour = rgb * lightResult;
    fFragColor = vec4(finalColour.x, finalColour.y, finalColour.z, 0.0f);

    // Common convention to take alpha from diffuse material.
    fFragColor.a = Object.uDiffuseAlbedo.a;
#endif // IS_WIREFRAME
}
