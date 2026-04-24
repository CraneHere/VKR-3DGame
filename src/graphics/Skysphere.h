#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Skysphere
{
public:
    Skysphere() = default;
    ~Skysphere();

    void Initialize(int segments = 32, int rings = 16);
    void Shutdown();

    // Рендеринг неба
    void Render(const glm::mat4& view, const glm::mat4& projection);

    // Настройки цветов неба
    void SetSunDirection(const glm::vec3& dir) { m_SunDirection = glm::normalize(dir); }
    void SetHorizonColor(const glm::vec3& color) { m_HorizonColor = color; }
    void SetZenithColor(const glm::vec3& color) { m_ZenithColor = color; }
    void SetSunColor(const glm::vec3& color) { m_SunColor = color; }
    void SetSunSize(float size) { m_SunSize = size; }

    // ImGui
    glm::vec3& GetHorizonColor() { return m_HorizonColor; }
    glm::vec3& GetZenithColor() { return m_ZenithColor; }
    glm::vec3& GetSunColor() { return m_SunColor; }
    float& GetSunSize() { return m_SunSize; }

private:
    void GenerateSphere(int segments, int rings);
    void LoadShader();

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    int m_IndexCount = 0;

    Shader m_Shader;

    // Цвета неба
    glm::vec3 m_SunDirection = glm::normalize(glm::vec3(-0.5f, -0.3f, -0.7f));
    glm::vec3 m_HorizonColor = glm::vec3(0.9f, 0.6f, 0.4f);   // Оранжевый горизонт
    glm::vec3 m_ZenithColor = glm::vec3(0.3f, 0.4f, 0.6f);    // Синее небо вверху
    glm::vec3 m_SunColor = glm::vec3(1.0f, 0.9f, 0.7f);       // Яркое солнце
    float m_SunSize = 0.05f;

    bool m_Initialized = false;
};
