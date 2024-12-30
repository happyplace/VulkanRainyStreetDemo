#ifndef VRSD_GameMapRenderer_h_
#define VRSD_GameMapRenderer_h_

struct GameMap;
struct GameMapRenderer;

GameMapRenderer* game_map_renderer_init();
void game_map_renderer_destroy(GameMapRenderer* game_map_renderer);

void game_map_renderer_render(GameMapRenderer* game_map_renderer, struct FrameResource* frame_resource, struct Game* game);
void game_map_renderer_imgui_draw();

#endif // VRSD_GameMapRenderer_h_
