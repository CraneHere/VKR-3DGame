#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct MeshTexture {
    unsigned int id;
    std::string type;
    std::string path; // проверка дубликатов при загрузке
};

class Mesh {
public:
    std::vector<Vertex>      vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshTexture> textures;

    Mesh(std::vector<Vertex> vertices,
         std::vector<unsigned int> indices,
         std::vector<MeshTexture> textures);

    void Draw(unsigned int shaderID) const;

    void Cleanup();

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    // Создаем VAO/VBO/EBO и настраиваем атрибуты вершин
    void setupMesh();
};
