#ifndef VRSD_GameTimer_h_
#define VRSD_GameTimer_h_

#include <cstdint>

struct GameTimer;

GameTimer* game_timer_init();
void game_timer_destroy(GameTimer* game_timer);

double game_timer_total_time(GameTimer* game_timer);
double game_timer_delta_time(GameTimer* game_timer);
uint64_t game_timer_frame_Count(GameTimer* game_timer);

void game_timer_tick(GameTimer* game_timer);

void game_timer_reset(GameTimer* game_timer);
void game_timer_start(GameTimer* game_timer);
void game_timer_stop(GameTimer* game_timer);

#endif // VRSD_GameTimer_h_
