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

struct VRSD_ALIGN(16) Vulkan_MeshRendererObjectBuffer
{
    DirectX::XMFLOAT4X4 world;
};

#endif // VRSD_RenderDefines_h_
