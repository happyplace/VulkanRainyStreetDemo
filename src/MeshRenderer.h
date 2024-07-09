#ifndef VRSD_MeshRenderer_h_
#define VRSD_MeshRenderer_h_

#include <DirectXMath.h>

#include "VrsdStd.h"

struct MeshRenderer;

struct VRSD_ALIGN(16) Vulkan_MeshRendererObjectBuffer
{
    DirectX::XMFLOAT4X4 world;
};

MeshRenderer* mesh_renderer_init(struct Game* game);
void mesh_renderer_destroy(MeshRenderer* mesh_renderer, struct Game* game);

#endif // VRSD_MeshRenderer_h_
