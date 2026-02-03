#include "Renderer.h"
#include <glad/glad.h>

void Renderer::Initialize()
{
    // Базовая инициализация OpenGL состояния
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void Renderer::Shutdown()
{
    // Очистка ресурсов при необходимости
}

void Renderer::BeginFrame()
{
    // Подготовка к рендерингу нового кадра
}

void Renderer::EndFrame()
{
    // Завершение кадра
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::DrawCube()
{

}

void Renderer::DrawSphere()
{

}

void Renderer::DrawPlane()
{

}
