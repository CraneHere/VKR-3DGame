#pragma once

#include <glm/glm.hpp>

// Направленный свет (солнце)
struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.5f, -0.3f, -0.7f);  // Закатное солнце
    glm::vec3 color = glm::vec3(1.0f, 0.7f, 0.4f);         // Теплый оранжевый
    float intensity = 1.0f;
};

// Полусферическое освещение (небо/земля)
struct AmbientLight {
    glm::vec3 skyColor = glm::vec3(0.6f, 0.5f, 0.5f);      // Верхний цвет
    glm::vec3 groundColor = glm::vec3(0.3f, 0.25f, 0.2f);  // Нижний цвет
    float intensity = 0.3f;
};

// Параметры атмосферного тумана
struct FogSettings {
    glm::vec3 color = glm::vec3(0.9f, 0.6f, 0.4f);  // Цвет заката
    float density = 0.015f;                          // Плотность
    float heightFalloff = 0.1f;                      // Затухание по высоте
    bool enabled = true;
};

class Renderer
{
public:
    static void Initialize();
    static void Shutdown();
    
    static void BeginFrame();
    static void EndFrame();
    
    static void Clear();
    static void SetClearColor(float r, float g, float b, float a = 1.0f);
    static void SetViewport(int x, int y, int width, int height);
    
    static void DrawCube();
    static void DrawSphere();
    static void DrawPlane();
};
