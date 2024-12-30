#ifndef VRSD_MeshRenderer_h_
#define VRSD_MeshRenderer_h_

#include <cstdint>

struct PhongMeshRenderer;

PhongMeshRenderer* phong_mesh_renderer_init(struct Game* game);
void phong_mesh_renderer_destroy(PhongMeshRenderer* mesh_renderer, struct Game* game);

void phong_mesh_renderer_render(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, struct Game* game);
void phong_mesh_renderer_imgui_draw();

uint32_t phong_mesh_renderer_get_object_buffer_offset(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, uint32_t object_index);

#endif // VRSD_MeshRenderer_h_
