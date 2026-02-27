#version 450 core

// Входные атрибуты вершин
layout(location = 0) in vec3 aPosition;

// Uniform переменные
uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main()
{
    // Трансформируем позицию в пространство света
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
