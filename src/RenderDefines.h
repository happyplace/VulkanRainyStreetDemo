#ifndef VRSD_RenderDefines_h_
#define VRSD_RenderDefines_h_

#include <DirectXMath.h>

#include "VrsdStd.h"

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT2 texture;
};

struct VRSD_ALIGN(16) DirectionalLight
{
    DirectX::XMFLOAT3 strength;
    DirectX::XMFLOAT3 direction;
};

struct VRSD_ALIGN(16) ObjectBuffer
{
    DirectX::XMFLOAT4X4 world;
};

struct VRSD_ALIGN(16) MeshRenderer_FrameBuffer
{
    DirectX::XMFLOAT4X4 view_proj;
    DirectX::XMFLOAT3 eye_pos;
    float padding0;
    DirectX::XMFLOAT4 ambient_light;
};

#endif // VRSD_RenderDefines_h_
