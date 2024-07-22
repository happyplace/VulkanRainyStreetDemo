#ifndef VRSD_MeshRenderer_h_
#define VRSD_MeshRenderer_h_

#include <DirectXMath.h>

#include "VrsdStd.h"

struct PhongMeshRenderer;

struct VRSD_ALIGN(16) Vulkan_MeshRendererObjectBuffer
{
    DirectX::XMFLOAT4X4 world;
};

PhongMeshRenderer* phong_mesh_renderer_init(struct Game* game);
void phong_mesh_renderer_destroy(PhongMeshRenderer* mesh_renderer, struct Game* game);

void phong_mesh_renderer_render(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, struct Game* game);

uint32_t phong_mesh_renderer_get_object_buffer_offset(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, uint32_t object_index);

#endif // VRSD_MeshRenderer_h_
