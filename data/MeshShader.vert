#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

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

layout(std140, set = 1, binding = 0) uniform ObjectBuf
{
    mat4 uWorld;
    vec4 uDiffuseAlbedo;
    vec3 uFresnelR0;
    float uRoughness;
    uint uTextureIndex;
} Object;

#ifdef USE_WATER_TEXTURE
    layout(set = 2, binding = 0) uniform sampler samplerColour;
    layout(set = 4, binding = 0) uniform texture2D sampledWaterHeightTexture;
#endif // USE_WATER_TEXTURE

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vNormalW;
layout(location = 1) out vec3 vPositionW;
layout(location = 2) out vec2 outUV;

void main()
{
    vec4 position = vec4(aPosition, 1.0f);
#ifndef DUCK_WATER_SAMPLE
#ifdef USE_WATER_TEXTURE
    position.y += texture(sampler2D(sampledWaterHeightTexture, samplerColour), inUV).x;
#endif // USE_WATER_TEXTURE 
#endif // DUCK_WATER_SAMPLE

    mat4 world = Object.uWorld;
#ifdef DUCK_WATER_SAMPLE
    float offsetY = texture(sampler2D(sampledWaterHeightTexture, samplerColour), vec2(0.5f, 0.5f)).x;
    offsetY -= 150.0;
    mat4 offsetMat;
    offsetMat[0].xyzw = vec4(1.0, 0.0, 0.0, 0.0);
    offsetMat[1].xyzw = vec4(0.0, 1.0, 0.0, offsetY);
    offsetMat[2].xyzw = vec4(0.0, 0.0, 1.0, 0.0);
    offsetMat[3].xyzw = vec4(0.0, 0.0, 0.0, 1.0);

    world = world * offsetMat;
#endif // DUCK_WATER_SAMPLE

    vec4 posW = position * world;
    vPositionW = posW.xyz;
    gl_Position = posW * Frame.uViewProj;

    vNormalW = aNormal * mat3(world);

    outUV = inUV; 
}
