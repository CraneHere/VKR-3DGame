#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Mesh.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiTexture;
enum aiTextureType : int;

class Model {
public:
    bool LoadFromFile(const std::string& path);
    void Draw(unsigned int shaderID) const;
    void Cleanup();

    const std::vector<glm::vec3>&  GetCollisionVertices() const { return m_collisionVertices; }
    const std::vector<unsigned int>& GetCollisionIndices() const { return m_collisionIndices; }
    glm::vec3 GetAABBMin() const { return m_aabbMin; }
    glm::vec3 GetAABBMax() const { return m_aabbMax; }

private:
    std::vector<Mesh>        m_meshes;
    std::vector<MeshTexture> m_loadedTextures;
    std::string              m_directory;

    std::vector<glm::vec3>    m_collisionVertices;
    std::vector<unsigned int> m_collisionIndices;
    glm::vec3 m_aabbMin{0.0f};
    glm::vec3 m_aabbMax{0.0f};

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<MeshTexture> loadMaterialTextures(
        aiMaterial* mat, aiTextureType type,
        const std::string& typeName, const aiScene* scene);

    unsigned int loadEmbeddedTexture(const aiTexture* tex);
    unsigned int loadTextureFromFile(const std::string& filepath);
};
