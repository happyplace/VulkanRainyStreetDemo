#ifndef VRSD_GameMapRenderer_h_
#define VRSD_GameMapRenderer_h_

struct GameMap;
struct GameMapRenderer;

GameMapRenderer* game_map_renderer_init();
void game_map_renderer_destroy(GameMapRenderer* game_map_renderer);

#endif // VRSD_GameMapRenderer_h_
