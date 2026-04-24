#include "Game.h"
#include "graphics/Primitives.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Game::Game()
{
}

Game::~Game()
{
    Shutdown();
}

bool Game::Initialize()
{
    if (m_Initialized) return true;

    // Загружаем шейдеры
    LoadShaders();

    // Создаём геометрию
    CreateGeometry();

    // Инициализируем shadow map
    m_ShadowMap.Initialize(2048, 2048);

    // Настройка камеры
    m_Camera.SetPosition(0.0f, 8.0f, 25.0f);
    m_Camera.SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 500.0f);

    // OpenGL настройки
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_Initialized = true;

    return true;
}

void Game::LoadShaders()
{
    // Основной шейдер с освещением
    if (!m_MainShader.LoadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag"))
    {
        std::cerr << "Failed to load main shader" << std::endl;
    }

    // Шейдер для shadow pass
    if (!m_ShadowShader.LoadFromFiles("assets/shaders/shadow.vert", "assets/shaders/shadow.frag"))
    {
        std::cerr << "Failed to load shadow shader" << std::endl;
    }
}

void Game::CreateGeometry()
{
    // Куб
    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);
    glGenBuffers(1, &m_CubeEBO);

    glBindVertexArray(m_CubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Пол
    glGenVertexArrays(1, &m_FloorVAO);
    glGenBuffers(1, &m_FloorVBO);
    glGenBuffers(1, &m_FloorEBO);

    glBindVertexArray(m_FloorVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_FloorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FLOOR_VERTICES), FLOOR_VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FloorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(FLOOR_INDICES), FLOOR_INDICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, FLOOR_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, FLOOR_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Game::Update(float deltaTime)
{
    m_Camera.Update(deltaTime);

    // Пересчитываем light space matrix
    m_LightSpaceMatrix = m_ShadowMap.CalculateLightSpaceMatrix(
        m_DirectionalLight.direction,
        m_SceneCenter,
        m_SceneRadius
    );
}

void Game::Render()
{
    if (!m_Initialized) return;

    RenderShadowPass();

    RenderMainPass();
}

void Game::RenderShadowPass()
{
    m_ShadowMap.Bind();
    m_ShadowMap.Clear();

    // Отключаем face culling для теней
    glCullFace(GL_FRONT);

    m_ShadowShader.Use();
    m_ShadowShader.SetMat4("uLightSpaceMatrix", glm::value_ptr(m_LightSpaceMatrix));

    // Рендер кубов
    glBindVertexArray(m_CubeVAO);
    for (int x = -2; x <= 2; x++)
    {
        for (int z = -2; z <= 2; z++)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * 4.0f, 1.0f, z * 4.0f));
            model = glm::scale(model, glm::vec3(2.0f));
            m_ShadowShader.SetMat4("uModel", glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
    }

    // Рендер стен
    glBindVertexArray(m_WallVAO);
    glm::mat4 wallModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
    m_ShadowShader.SetMat4("uModel", glm::value_ptr(wallModel));
    glDrawElements(GL_TRIANGLES, WALL_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glCullFace(GL_BACK);
    m_ShadowMap.Unbind();
}

void Game::RenderMainPass()
{
    // Цвет фона - цвет тумана для плавного перехода
    glClearColor(m_FogSettings.color.r, m_FogSettings.color.g, m_FogSettings.color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_MainShader.Use();

    // Матрицы камеры
    m_MainShader.SetMat4("uView", m_Camera.GetViewMatrix());
    m_MainShader.SetMat4("uProjection", m_Camera.GetProjectionMatrix());
    m_MainShader.SetMat4("uLightSpaceMatrix", glm::value_ptr(m_LightSpaceMatrix));

    // Позиция камеры
    float camX, camY, camZ;
    m_Camera.GetPosition(camX, camY, camZ);
    m_MainShader.SetVec3("uViewPos", camX, camY, camZ);

    // Настройка освещения
    SetupLightingUniforms(m_MainShader);

    // Привязываем shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMap.GetDepthTexture());
    m_MainShader.SetInt("uShadowMap", 1);
    m_MainShader.SetBool("uShadowsEnabled", true);

    // Текстуры не используем
    m_MainShader.SetBool("uUseTexture", false);

    glBindVertexArray(m_FloorVAO);
    glm::mat4 floorModel = glm::mat4(1.0f);
    m_MainShader.SetMat4("uModel", glm::value_ptr(floorModel));
    m_MainShader.SetVec3("uObjectColor", 0.4f, 0.35f, 0.3f);
    glDrawElements(GL_TRIANGLES, FLOOR_INDEX_COUNT, GL_UNSIGNED_INT, 0);
    glBindVertexArray(m_CubeVAO);

    for (int x = -2; x <= 2; x++)
    {
        for (int z = -2; z <= 2; z++)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * 4.0f, 1.0f, z * 4.0f));
            model = glm::scale(model, glm::vec3(2.0f));
            m_MainShader.SetMat4("uModel", glm::value_ptr(model));
            
            // Вариация цвета для разных кубов
            float colorVar = ((x + 2) * 5 + (z + 2)) / 25.0f;
            m_MainShader.SetVec3("uObjectColor", 
                0.5f + colorVar * 0.1f, 
                0.45f + colorVar * 0.05f, 
                0.4f);
            
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(m_WallVAO);
    glm::mat4 wallModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
    m_MainShader.SetMat4("uModel", glm::value_ptr(wallModel));
    m_MainShader.SetVec3("uObjectColor", 0.6f, 0.55f, 0.5f); // Светлый камень
    glDrawElements(GL_TRIANGLES, WALL_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void Game::SetupLightingUniforms(Shader& shader)
{
    // Directional Light
    shader.SetVec3("uLightDir", 
        m_DirectionalLight.direction.x, 
        m_DirectionalLight.direction.y, 
        m_DirectionalLight.direction.z);
    shader.SetVec3("uLightColor", 
        m_DirectionalLight.color.r, 
        m_DirectionalLight.color.g, 
        m_DirectionalLight.color.b);
    shader.SetFloat("uLightIntensity", m_DirectionalLight.intensity);

    // Hemisphere Ambient
    shader.SetVec3("uSkyColor", 
        m_AmbientLight.skyColor.r, 
        m_AmbientLight.skyColor.g, 
        m_AmbientLight.skyColor.b);
    shader.SetVec3("uGroundColor", 
        m_AmbientLight.groundColor.r, 
        m_AmbientLight.groundColor.g, 
        m_AmbientLight.groundColor.b);
    shader.SetFloat("uAmbientIntensity", m_AmbientLight.intensity);

    // Fog
    shader.SetVec3("uFogColor", 
        m_FogSettings.color.r, 
        m_FogSettings.color.g, 
        m_FogSettings.color.b);
    shader.SetFloat("uFogDensity", m_FogSettings.density);
    shader.SetFloat("uFogHeightFalloff", m_FogSettings.heightFalloff);
    shader.SetBool("uFogEnabled", m_FogSettings.enabled);
}

void Game::Shutdown()
{
    if (!m_Initialized) return;

    // Удаляем VAO/VBO/EBO
    glDeleteVertexArrays(1, &m_CubeVAO);
    glDeleteBuffers(1, &m_CubeVBO);
    glDeleteBuffers(1, &m_CubeEBO);

    glDeleteVertexArrays(1, &m_FloorVAO);
    glDeleteBuffers(1, &m_FloorVBO);
    glDeleteBuffers(1, &m_FloorEBO);

    glDeleteVertexArrays(1, &m_WallVAO);
    glDeleteBuffers(1, &m_WallVBO);
    glDeleteBuffers(1, &m_WallEBO);

    m_ShadowMap.Shutdown();

    m_Initialized = false;
}