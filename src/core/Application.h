#pragma once

class Application
{
public:
    Application();
    ~Application();

    int Run();

private:
    bool Initialize();
    void MainLoop();
    void Shutdown();

private:
    bool m_Running = false;
};
