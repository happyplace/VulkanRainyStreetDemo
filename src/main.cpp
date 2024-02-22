#include <iostream>

#include "Window.h"

int main(int argc, char** argv)
{
    Window window;

    if (!window.Init(argc, argv))
    {
        return 1;
    }

    window.WaitAndProcessEvents();

    return 0;
}

