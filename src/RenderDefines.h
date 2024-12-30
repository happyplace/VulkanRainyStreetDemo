#ifndef VRSD_RenderDefines_h_
#define VRSD_RenderDefines_h_

#include <DirectXMath.h>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT2 texture;
};

struct DirectionalLight
{
    DirectX::XMFLOAT3 rotation_euler;
    DirectX::XMFLOAT3 strength;
};

struct PointLight
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 strength;
    float falloff_start;
    float falloff_end;
};

struct SpotLight
{
    DirectX::XMFLOAT3 rotation_euler;
    DirectX::XMFLOAT3 strength;
    DirectX::XMFLOAT3 position;
    float falloff_start;
    float falloff_end;
    float spot_power;
};

struct Light
{
    DirectX::XMFLOAT3 strength;
    float falloff_start; // point/spot light only
    DirectX::XMFLOAT3 direction;   // directional/spot light only
    float falloff_end;   // point/spot light only
    DirectX::XMFLOAT3 position;    // point light only
    float spot_power;    // spot light only
};

struct Material
{
    DirectX::XMFLOAT4 diffuse_albedo;
    DirectX::XMFLOAT3 fresnelR0;
    float shininess;
};

struct ObjectBuffer
{
    DirectX::XMFLOAT4X4 world;
    Material material;
};

constexpr uint32_t c_render_defines_max_lights = 16;

struct MeshRenderer_FrameBuffer
{
    DirectX::XMFLOAT4X4 view_proj;
    DirectX::XMFLOAT3 eye_pos;
    float padding0;
    DirectX::XMFLOAT4 ambient_light;
    Light lights[c_render_defines_max_lights];
};

#endif // VRSD_RenderDefines_h_
