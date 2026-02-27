#version 450 core

// Входные атрибуты вершин
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// Uniform переменные (матрицы трансформации)
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;  // Для теней

// Выходные данные для фрагментного шейдера
out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vFragPosLightSpace;  // Позиция в пространстве света для теней

void main()
{
    // Позиция фрагмента в мировых координатах
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vFragPos = vec3(worldPos);
    
    // Нормаль с учётом трансформации модели (для non-uniform scaling)
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    
    // Текстурные координаты
    vTexCoord = aTexCoord;
    
    // Позиция в пространстве света (для shadow mapping)
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    
    // Финальная позиция вершины
    gl_Position = uProjection * uView * worldPos;
}
