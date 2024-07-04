#ifndef VRSD_GameMap_h_
#define VRSD_GameMap_h_

struct GameMapRenderer;

struct GameMap
{
    GameMapRenderer* map_renderer = nullptr;
};

GameMap* game_map_init();
void game_map_destroy(GameMap* game_map);

void game_map_load(GameMap* game_map);

void game_map_update(struct Game* game, struct FrameResource* frame_resource);
void game_map_render(struct Game* game, struct FrameResource* frame_resource);

#endif // VRSD_GameMap_h_
