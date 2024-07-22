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

void mesh_renderer_render(MeshRenderer* mesh_renderer, struct FrameResource* frame_resource, struct Game* game);

uint32_t mesh_renderer_get_object_buffer_offset(MeshRenderer* mesh_renderer, struct FrameResource* frame_resource, uint32_t object_index);

#endif // VRSD_MeshRenderer_h_
