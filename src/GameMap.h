#ifndef VRSD_GameMap_h_
#define VRSD_GameMap_h_

struct GameMapRenderer;

struct GameMap
{
    GameMapRenderer* map_renderer = nullptr;
};

GameMap* game_map_init();
void game_map_destroy(GameMap* game_map);

#endif // VRSD_GameMap_h_
