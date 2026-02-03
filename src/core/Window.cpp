#include "Window.h"
#include "Input.h"
#include <SDL.h>
#include <glad/glad.h>
#include <imgui_impl_sdl2.h>
#include <iostream>

Window::Window(const std::string& title, int width, int height)
    : m_Title(title), m_Width(width), m_Height(height)
{
}

Window::~Window()
{
    if (m_GLContext)
    {
        SDL_GL_DeleteContext(m_GLContext);
    }
    if (m_Window)
    {
        SDL_DestroyWindow(m_Window);
    }
    SDL_Quit();
}

bool Window::Create()
{
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // OpenGL атрибуты
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Создание окна
    m_Window = SDL_CreateWindow(
        m_Title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_Width,
        m_Height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!m_Window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Создание OpenGL контекста
    m_GLContext = SDL_GL_CreateContext(m_Window);
    if (!m_GLContext)
    {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(m_Window, m_GLContext);
    SDL_GL_SetSwapInterval(1);

    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    return true;
}

void Window::SwapBuffers()
{
    SDL_GL_SwapWindow(m_Window);
}

void Window::ProcessEvents()
{
    Input::Update();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // ImGui обрабатывает события
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_QUIT:
                m_ShouldClose = true;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    m_Width = event.window.data1;
                    m_Height = event.window.data2;
                    glViewport(0, 0, m_Width, m_Height);
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    Input::SetMouseCaptured(!Input::IsMouseCaptured());
                }
                break;

            case SDL_MOUSEMOTION:
                if (Input::IsMouseCaptured())
                {

                }
                break;
        }
    }
}
