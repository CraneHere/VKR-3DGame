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

    // Создаём геометрию
    CreateGeometry();

    // Настройка камеры
    m_Camera.SetPosition(0.0f, 8.0f, 25.0f);
    m_Camera.SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 500.0f);

    // OpenGL настройки
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_Initialized = true;
    std::cout << "=== Game Initialized ===" << std::endl;

    return true;
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
}

void Game::Render()
{
    if (!m_Initialized) return;

    // Очистка экрана
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Простой рендеринг без шейдеров (wireframe для отладки)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(m_FloorVAO);
    glDrawElements(GL_TRIANGLES, FLOOR_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glBindVertexArray(m_CubeVAO);
    for (int x = -2; x <= 2; x++)
    {
        for (int z = -2; z <= 2; z++)
        {
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}

void Game::Shutdown()
{
    if (!m_Initialized) return;

    glDeleteVertexArrays(1, &m_CubeVAO);
    glDeleteBuffers(1, &m_CubeVBO);
    glDeleteBuffers(1, &m_CubeEBO);

    glDeleteVertexArrays(1, &m_FloorVAO);
    glDeleteBuffers(1, &m_FloorVBO);
    glDeleteBuffers(1, &m_FloorEBO);

    m_Initialized = false;
}
