#pragma once

class Window
{
public:
    Window();
    ~Window();

    // create and open window, returns true on success.
    // if this function fails application should be terminated
    bool Init();

    // this function should be called AFTER the job system has been started and the first job has
    // been pushed. This will wait for a window event and then process it.
    // This should be called on the MAIN THREAD and will NOT return until the application is quit
    void WaitAndProcessEvents();

private:
    void Destroy();

    class WindowImpl* m_impl = nullptr; 
};

