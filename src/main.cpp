#include "Window.h"
#include "Singleton.h"
#include "GameCommandParameters.h"

int main(int argc, char** argv)
{
    Singleton<GameCommandParameters>::Create();
    Singleton<GameCommandParameters>::Get()->SetValues(argc, argv);

    Singleton<Window>::Create();
    if (!Singleton<Window>::Get()->Init())
    {
        return 1;
    }

    Singleton<Window>::Destroy();
    Singleton<GameCommandParameters>::Destroy();

    return 0;
}
