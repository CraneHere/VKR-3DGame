#include "Skysphere.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <iostream>

// Шейдер для Skysphere
const char* SKY_VERTEX_SHADER = R"(
#version 450 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;

void main()
{
    vWorldPos = aPosition;
    
    // Убираем позицию камеры из view матрицы (небо всегда вокруг камеры)
    mat4 viewNoTranslation = mat4(mat3(uView));
    
    vec4 pos = uProjection * viewNoTranslation * vec4(aPosition, 1.0);
    
    // z = w чтобы небо было на максимальной глубине
    gl_Position = pos.xyww;
}
)";

const char* SKY_FRAGMENT_SHADER = R"(
#version 450 core

in vec3 vWorldPos;

uniform vec3 uSunDirection;
uniform vec3 uHorizonColor;
uniform vec3 uZenithColor;
uniform vec3 uSunColor;
uniform float uSunSize;

out vec4 FragColor;

// Dithering для устранения banding
float dither(vec2 screenPos)
{
    // Bayer matrix 4x4 dithering
    int x = int(mod(screenPos.x, 4.0));
    int y = int(mod(screenPos.y, 4.0));
    int index = x + y * 4;
    float bayerMatrix[16] = float[16](
        0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
        12.0/16.0, 4.0/16.0, 14.0/16.0,  6.0/16.0,
        3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
        15.0/16.0, 7.0/16.0, 13.0/16.0,  5.0/16.0
    );
    return (bayerMatrix[index] - 0.5) / 255.0;
}

void main()
{
    vec3 dir = normalize(vWorldPos);
    
    // Высота над горизонтом (0 = горизонт, 1 = зенит)
    float height = dir.y;
    
    // Плавный градиент неба: горизонт -> зенит
    float t = smoothstep(-0.2, 0.6, height);
    vec3 skyColor = mix(uHorizonColor, uZenithColor, t);
    
    // Мягкое свечение у горизонта
    float horizonGlow = 1.0 - abs(height);
    horizonGlow = pow(horizonGlow, 4.0);
    skyColor = mix(skyColor, uHorizonColor * 1.1, horizonGlow * 0.4);
    
    // Солнце
    vec3 sunDir = -normalize(uSunDirection);
    float sunDot = dot(dir, sunDir);
    
    // Мягкий диск солнца
    float sunDisc = smoothstep(1.0 - uSunSize, 1.0 - uSunSize * 0.3, sunDot);
    
    // Плавное свечение вокруг солнца
    float sunGlow = pow(max(sunDot, 0.0), 6.0);
    float sunGlow2 = pow(max(sunDot, 0.0), 32.0); // Яркий центр
    
    // Финальный цвет
    vec3 finalColor = skyColor;
    finalColor += uSunColor * sunGlow * 0.4;         // Широкое свечение
    finalColor += uSunColor * sunGlow2 * 0.3;        // Яркий центр
    finalColor = mix(finalColor, uSunColor, sunDisc); // Диск солнца
    
    // Dithering для устранения полос на градиенте
    float ditherValue = dither(gl_FragCoord.xy);
    finalColor += vec3(ditherValue);
    
    FragColor = vec4(finalColor, 1.0);
}
)";

Skysphere::~Skysphere()
{
    Shutdown();
}

void Skysphere::Initialize(int segments, int rings)
{
    if (m_Initialized)
    {
        Shutdown();
    }

    GenerateSphere(segments, rings);
    LoadShader();

    m_Initialized = true;
    std::cout << "Skysphere initialized" << std::endl;
}

void Skysphere::GenerateSphere(int segments, int rings)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Генерация вершин сферы
    for (int ring = 0; ring <= rings; ring++)
    {
        float phi = 3.14159265359f * ring / rings;
        float y = cos(phi);
        float sinPhi = sin(phi);

        for (int seg = 0; seg <= segments; seg++)
        {
            float theta = 2.0f * 3.14159265359f * seg / segments;
            float x = sinPhi * cos(theta);
            float z = sinPhi * sin(theta);

            // Небо далеко - радиус большой
            float radius = 500.0f;
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }

    // Генерация индексов
    for (int ring = 0; ring < rings; ring++)
    {
        for (int seg = 0; seg < segments; seg++)
        {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    m_IndexCount = static_cast<int>(indices.size());

    // VAO/VBO/EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Skysphere::LoadShader()
{
    if (!m_Shader.LoadFromSource(SKY_VERTEX_SHADER, SKY_FRAGMENT_SHADER))
    {
        std::cerr << "Failed to load skysphere shader!" << std::endl;
    }
}

void Skysphere::Render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_Initialized) return;

    // Небо первым, с отключенной записью в depth buffer
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE); // Рендер изнутри сферы

    m_Shader.Use();
    m_Shader.SetMat4("uView", glm::value_ptr(view));
    m_Shader.SetMat4("uProjection", glm::value_ptr(projection));
    m_Shader.SetVec3("uSunDirection", m_SunDirection.x, m_SunDirection.y, m_SunDirection.z);
    m_Shader.SetVec3("uHorizonColor", m_HorizonColor.r, m_HorizonColor.g, m_HorizonColor.b);
    m_Shader.SetVec3("uZenithColor", m_ZenithColor.r, m_ZenithColor.g, m_ZenithColor.b);
    m_Shader.SetVec3("uSunColor", m_SunColor.r, m_SunColor.g, m_SunColor.b);
    m_Shader.SetFloat("uSunSize", m_SunSize);

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Восстанавливаем состояние
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

void Skysphere::Shutdown()
{
    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO != 0)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_EBO != 0)
    {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }

    m_Initialized = false;
}
