#pragma once

#include <SDL_assert.h>
#include <SDL_log.h>
//#include <SDL_messagebox.h>

struct VkAllocationCallbacks;

#define GAME_SHOW_ERROR(title, message) \
    /* MessageBox need to be called from the main thread or the thread that created the window so it's not safe to be calling this function from fibers, for now we're just going to log the message only */ \
    /* if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, DuckDemoUtils::GetWindow()) < 0)*/ \
    { \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\n\t[Title]   %s\n\t[Message] %s", title, message); \
    }

#define GAME_ASSERT SDL_assert

