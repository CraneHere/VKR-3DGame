#include "Application.h"
#include "Window.h"
#include "Input.h"
#include "game/Game.h"
#include "graphics/Renderer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL.h>
#include <iostream>
#include <memory>

namespace
{
    std::unique_ptr<Window> s_Window;
    std::unique_ptr<Game> s_Game;
}

Application::Application()
{
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    std::cout << "=== VKR 3D Game - ODM Ground Movement ===" << std::endl;

    // Создаём окно
    s_Window = std::make_unique<Window>("VKR 3D Game - ODM Ground Movement", 1600, 900);
    if (!s_Window->Create())
    {
        std::cerr << "Failed to create window!" << std::endl;
        return false;
    }

    // Инициализируем ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(s_Window->GetSDLWindow(), s_Window->GetGLContext());
    ImGui_ImplOpenGL3_Init("#version 450");

    // Инициализируем рендерер
    Renderer::Initialize();

    // Инициализируем игру
    s_Game = std::make_unique<Game>();
    if (!s_Game->Initialize())
    {
        std::cerr << "Failed to initialize game!" << std::endl;
        return false;
    }

    m_Running = true;
    return true;
}

void Application::MainLoop()
{
    Uint64 lastTime = SDL_GetPerformanceCounter();
    float deltaTime = 0.0f;

    while (m_Running && !s_Window->ShouldClose())
    {
        // Время кадра
        Uint64 currentTime = SDL_GetPerformanceCounter();
        deltaTime = static_cast<float>(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        // Обработка событий
        s_Window->ProcessEvents();

        // ImGui новый кадр
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Простой UI
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("Press ESC to toggle mouse capture");
        ImGui::Text("WASD - move, Space - jump");
        ImGui::Text("LMB/RMB - fire grapples");
        ImGui::Text("Shift + WASD - ground reel movement");
        ImGui::End();

        // Обновление игры
        s_Game->Update(deltaTime);

        // Рендеринг
        s_Game->Render();

        // Рендеринг ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        s_Window->SwapBuffers();
    }
}

void Application::Shutdown()
{
    if (s_Game)
    {
        s_Game->Shutdown();
        s_Game.reset();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    Renderer::Shutdown();

    s_Window.reset();
}

int Application::Run()
{
    if (!Initialize())
    {
        return -1;
    }

    MainLoop();
    Shutdown();

    return 0;
}
