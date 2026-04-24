#pragma once

#include "graphics/Renderer.h"
#include "graphics/Shader.h"
#include "graphics/Camera.h"
#include "graphics/ShadowMap.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Game
{
public:
    Game();
    ~Game();

    bool Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    // Доступ к настройкам освещения для ImGui
    DirectionalLight& GetDirectionalLight() { return m_DirectionalLight; }
    AmbientLight& GetAmbientLight() { return m_AmbientLight; }
    FogSettings& GetFogSettings() { return m_FogSettings; }

private:
    void LoadShaders();
    void CreateGeometry();
    void RenderShadowPass();
    void RenderMainPass();
    void SetupLightingUniforms(Shader& shader);

private:
    // Шейдеры
    Shader m_MainShader;
    Shader m_ShadowShader;

    // Камера
    Camera m_Camera;

    // Shadow mapping
    ShadowMap m_ShadowMap;
    glm::mat4 m_LightSpaceMatrix;

    // Освещение
    DirectionalLight m_DirectionalLight;
    AmbientLight m_AmbientLight;
    FogSettings m_FogSettings;

    // Геометрия сцены
    GLuint m_CubeVAO = 0, m_CubeVBO = 0, m_CubeEBO = 0;
    GLuint m_FloorVAO = 0, m_FloorVBO = 0, m_FloorEBO = 0;
    GLuint m_WallVAO = 0, m_WallVBO = 0, m_WallEBO = 0;

    // Параметры сцены
    glm::vec3 m_SceneCenter = glm::vec3(0.0f, 5.0f, 0.0f);
    float m_SceneRadius = 60.0f;

    bool m_Initialized = false;
};
