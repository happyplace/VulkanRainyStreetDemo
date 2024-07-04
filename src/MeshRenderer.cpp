#include "MeshRenderer.h"

#include <SDL_assert.h>

MeshRenderer* mesh_renderer_init(struct Game* game)
{
    MeshRenderer* mesh_renderer = new MeshRenderer();

    return mesh_renderer;
}

void mesh_renderer_destroy(MeshRenderer* mesh_renderer, struct Game* game)
{
    SDL_assert(mesh_renderer);

    delete mesh_renderer;
}
