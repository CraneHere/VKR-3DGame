#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ShadowMap
{
public:
    ShadowMap() = default;
    ~ShadowMap();

    void Initialize(int width = 2048, int height = 2048);
    void Shutdown();

    void Bind();
    void Unbind();

    void Clear();

    // Получение текстуры глубины для использования в основном шейдере
    GLuint GetDepthTexture() const { return m_DepthTexture; }

    // Матрица пространства света
    glm::mat4 CalculateLightSpaceMatrix(
        const glm::vec3& lightDir,
        const glm::vec3& sceneCenter,
        float sceneRadius
    ) const;

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    bool IsInitialized() const { return m_Initialized; }

private:
    GLuint m_FBO = 0;
    GLuint m_DepthTexture = 0;
    int m_Width = 2048;
    int m_Height = 2048;
    bool m_Initialized = false;

    // Сохранённый viewport для восстановления после рендеринга теней
    GLint m_PreviousViewport[4] = {0, 0, 0, 0};
};
