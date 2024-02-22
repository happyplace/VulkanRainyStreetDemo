#include "Window.h"

#include <SDL.h>

#include "GameUtils.h"

class WindowImpl
{
public:
    SDL_Window* m_window = nullptr;
};

Window::Window()
{
    m_impl = new WindowImpl();
}

Window::~Window()
{
    Destroy();
    delete m_impl;
}

void Window::Destroy()
{
    if (m_impl->m_window != nullptr)
    {
	SDL_DestroyWindow(m_impl->m_window);
    }

    SDL_Quit();
}

bool Window::Init(int /*argc*/, char** /*argv*/)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
	GAME_SHOW_ERROR("SDL Failed Init", SDL_GetError());
        return false;
    }

    m_impl->m_window = SDL_CreateWindow(
        "Vulkan Rainy Street Demo", 
	SDL_WINDOWPOS_UNDEFINED,
	SDL_WINDOWPOS_UNDEFINED, 
	1280, 720, 
	SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (m_impl->m_window == nullptr)
    {
	GAME_SHOW_ERROR("Window Creation Failure", SDL_GetError());
	return false;
    }

    return true;
}

void Window::WaitAndProcessEvents()
{
    bool quit = false;
    while (!quit)
    {
	SDL_Event event;
        if (SDL_WaitEvent(&event) != 1)
	{
	    GAME_SHOW_ERROR("Error Waiting on Window Event", SDL_GetError());
	    quit = true;
	}
	else
	{
	    if (event.type == SDL_QUIT)
	    {
		quit = true;
	    }
	}
    }
}

