#pragma once

#include "graphics/Camera.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

class Game
{
public:
    Game();
    ~Game();

    bool Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    Camera& GetCamera() { return m_Camera; }

private:
    void CreateGeometry();

private:
    // Камера
    Camera m_Camera;

    // Геометрия сцены
    GLuint m_CubeVAO = 0, m_CubeVBO = 0, m_CubeEBO = 0;
    GLuint m_FloorVAO = 0, m_FloorVBO = 0, m_FloorEBO = 0;

    bool m_Initialized = false;
};
