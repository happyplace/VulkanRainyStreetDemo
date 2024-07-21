#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

// layout(std140, set = 0, binding = 0) uniform FrameBuffer
// {
//     mat4 view_proj;
//     vec3 eye_pos;
// } Frame;

layout(std140, set = 0 /*1*/ , binding = 0) uniform ObjectBuffer
{
    mat4 world_view_proj;
} Object;

layout(location = 0) in vec3 in_positionL;
// layout(location = 1) in vec3 in_normal;
// layout(location = 2) in vec2 in_uv;

//layout(location = 0) out vec3 out_normalW;
// layout(location = 1) out vec3 out_positionW;
// layout(location = 2) out vec2 out_uv;

void main()
{
    // Transform to homogeneus clip space
    gl_Position = vec4(in_positionL, 1.0f) * Object.world_view_proj;
}
