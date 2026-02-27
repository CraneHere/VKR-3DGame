#pragma once

#include <string>

struct SDL_Window;
typedef void* SDL_GLContext;

class Window
{
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    bool Create();
    void SwapBuffers();
    void ProcessEvents();

    bool ShouldClose() const { return m_ShouldClose; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    float GetAspectRatio() const { return static_cast<float>(m_Width) / m_Height; }
    
    SDL_Window* GetSDLWindow() const { return m_Window; }
    SDL_GLContext GetGLContext() const { return m_GLContext; }

private:
    std::string m_Title;
    int m_Width;
    int m_Height;
    bool m_ShouldClose = false;

    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_GLContext = nullptr;
};
