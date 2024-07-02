#include "GameTimer.h"

#include <chrono>

#include <SDL_assert.h>

using namespace std::chrono;

typedef std::chrono::milliseconds::rep TimePoint;

constexpr double c_milliseconds_to_seconds = 0.001;

struct GameTimer
{
	bool is_stopped = false;
	double delta_time = 0.0;
	TimePoint base_time = 0;
	TimePoint paused_time = 0;
	TimePoint stop_time = 0;
	TimePoint previous_time = 0;
	TimePoint current_time = 0;
	uint64_t frame_count = 0;
};

void game_timer_destroy(GameTimer* game_timer)
{
    SDL_assert(game_timer);

    delete game_timer;
}

GameTimer* game_timer_init()
{
    GameTimer* game_timer = new GameTimer();

    return game_timer;
}

double game_timer_total_time(GameTimer* game_timer)
{
    if (game_timer->is_stopped)
    {
        return static_cast<double>(((game_timer->stop_time - game_timer->paused_time) - game_timer->base_time) * c_milliseconds_to_seconds);
    }
    else
    {
        return static_cast<double>(((game_timer->current_time - game_timer->paused_time) - game_timer->base_time) * c_milliseconds_to_seconds);
    }
}

TimePoint game_timer_get_current_time()
{
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

double game_timer_delta_time(GameTimer* game_timer)
{
    return game_timer->delta_time;
}

uint64_t game_timer_frame_Count(GameTimer* game_timer)
{
    return game_timer->frame_count;
}

void game_timer_tick(GameTimer* game_timer)
{
    if (game_timer->is_stopped)
    {
        game_timer->delta_time = 0.0;
        return;
    }

    game_timer->current_time = game_timer_get_current_time();

    game_timer->delta_time = (game_timer->current_time - game_timer->previous_time) * c_milliseconds_to_seconds;

    game_timer->previous_time = game_timer->current_time;

    if (game_timer->delta_time < 0.0)
    {
        game_timer->delta_time = 0.0;
    }

    game_timer->frame_count++;
}

void game_timer_reset(GameTimer* game_timer)
{
    TimePoint current_time = game_timer_get_current_time();

    game_timer->base_time = current_time;
    game_timer->previous_time = current_time;
    game_timer->stop_time = 0;
    game_timer->is_stopped = false;
}

void game_timer_start(GameTimer* game_timer)
{
    TimePoint start_time = game_timer_get_current_time();

    if (game_timer->is_stopped)
    {
        game_timer->paused_time += (start_time - game_timer->stop_time);

        game_timer->previous_time = start_time;
        game_timer->stop_time = 0;
        game_timer->is_stopped = false;
    }
}

void game_timer_stop(GameTimer* game_timer)
{
    if (!game_timer->is_stopped)
    {
        TimePoint current_time = game_timer_get_current_time();

        game_timer->stop_time = current_time;
        game_timer->is_stopped = true;
    }
}
