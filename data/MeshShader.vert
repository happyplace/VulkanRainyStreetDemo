#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, set = 0, binding = 0) uniform FrameBuffer
{
    mat4 view_projection;
    vec3 eye_position;
} Frame;

layout(std140, set = 1, binding = 0) uniform ObjectBuffer
{
    mat4 world;
} Object;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_normalW;
layout(location = 1) out vec3 out_positionW;
layout(location = 2) out vec2 out_uv;

void main()
{
    vec4 position = vec4(in_position, 1.0f);

    vec4 posW = position * Object.world;
    out_positionW = posW.xyz;
    gl_Position = posW * Frame.view_projection;

    out_normalW = in_normal * mat3(Object.world);

    out_uv = in_uv;
}
