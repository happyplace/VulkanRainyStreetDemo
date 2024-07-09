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

layout(location = 0) in vec3 in_normalW;
layout(location = 1) in vec3 in_positionW;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_frag_color;

void main()
{
    out_frag_color = vec4(1.0f, 1.0f, 1.0f, 0.0f);
}
