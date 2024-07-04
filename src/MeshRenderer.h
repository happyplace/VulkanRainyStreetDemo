#ifndef VRSD_MeshRenderer_h_
#define VRSD_MeshRenderer_h_

struct MeshRenderer
{

};

MeshRenderer* mesh_renderer_init(struct Game* game);
void mesh_renderer_destroy(MeshRenderer* mesh_renderer, struct Game* game);

#endif // VRSD_MeshRenderer_h_
