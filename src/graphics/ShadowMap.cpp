#include "ShadowMap.h"
#include <iostream>

ShadowMap::~ShadowMap()
{
    Shutdown();
}

void ShadowMap::Initialize(int width, int height)
{
    if (m_Initialized)
    {
        Shutdown();
    }

    m_Width = width;
    m_Height = height;

    // Создаём FBO
    glGenFramebuffers(1, &m_FBO);

    // Создаём текстуру глубины
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 
                 m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // Параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Белая граница для областей вне shadow map (нет тени)
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Привязываем текстуру к FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
    
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Проверяем полноту FBO
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ShadowMap: Framebuffer is not complete! Status: " << status << std::endl;
        Shutdown();
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_Initialized = true;
    std::cout << "ShadowMap initialized: " << m_Width << "x" << m_Height << std::endl;
}

void ShadowMap::Shutdown()
{
    if (m_FBO != 0)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }

    if (m_DepthTexture != 0)
    {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }

    m_Initialized = false;
}

void ShadowMap::Bind()
{
    if (!m_Initialized) return;

    // Сохраняем текущий viewport
    glGetIntegerv(GL_VIEWPORT, m_PreviousViewport);

    // Привязываем FBO и устанавливаем viewport под размер shadow map
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
}

void ShadowMap::Unbind()
{
    // Восстанавливаем default framebuffer и viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_PreviousViewport[0], m_PreviousViewport[1], 
               m_PreviousViewport[2], m_PreviousViewport[3]);
}

void ShadowMap::Clear()
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

glm::mat4 ShadowMap::CalculateLightSpaceMatrix(
    const glm::vec3& lightDir,
    const glm::vec3& sceneCenter,
    float sceneRadius) const
{
    // Нормализуем направление света
    glm::vec3 lightDirNorm = glm::normalize(lightDir);

    glm::vec3 lightPos = sceneCenter - lightDirNorm * sceneRadius * 2.0f;

    // View матрица света (ортогональная проекция смотрит в направлении света)
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    // Ортографическая проекция для directional light
    // Размер зависит от радиуса сцены
    float orthoSize = sceneRadius * 1.5f;
    glm::mat4 lightProjection = glm::ortho(
        -orthoSize, orthoSize,   // лево, право
        -orthoSize, orthoSize,   // низ, вверх
        0.1f, sceneRadius * 4.0f // близко, далеко
    );

    return lightProjection * lightView;
}
