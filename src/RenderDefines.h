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

#endif // VRSD_RenderDefines_h_
