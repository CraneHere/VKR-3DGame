#include "Model.h"

#include <iostream>
#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool Model::LoadFromFile(const std::string& path)
{
    Assimp::Importer importer;

    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    m_directory = path.substr(0, path.find_last_of("/\\"));

    processNode(scene->mRootNode, scene);

    // Собираем все вершины/индексы для физической коллизии
    unsigned int vertexOffset = 0;
    for (const auto& mesh : m_meshes) {
        for (const auto& v : mesh.vertices)
            m_collisionVertices.push_back(v.position);
        for (unsigned int idx : mesh.indices)
            m_collisionIndices.push_back(idx + vertexOffset);
        vertexOffset += static_cast<unsigned int>(mesh.vertices.size());
    }

    // Вычисляем AABB
    m_aabbMin = glm::vec3(FLT_MAX);
    m_aabbMax = glm::vec3(-FLT_MAX);
    for (const auto& pos : m_collisionVertices) {
        m_aabbMin = glm::min(m_aabbMin, pos);
        m_aabbMax = glm::max(m_aabbMax, pos);
    }

    std::cout << "Model loaded: " << path
              << " (" << m_meshes.size() << " meshes, "
              << m_collisionVertices.size() << " collision verts)" << std::endl;
    return true;
}

void Model::Draw(unsigned int shaderID) const
{
    for (const auto& mesh : m_meshes)
        mesh.Draw(shaderID);
}

void Model::Cleanup()
{
    for (auto& mesh : m_meshes)
        mesh.Cleanup();

    for (auto& tex : m_loadedTextures) {
        if (tex.id) {
            glDeleteTextures(1, &tex.id);
            tex.id = 0;
        }
    }

    m_meshes.clear();
    m_loadedTextures.clear();
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshTexture>  textures;

    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v{};

        v.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        if (mesh->HasNormals()) {
            v.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        if (mesh->mTextureCoords[0]) {
            v.texCoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto diffuse = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuse.begin(), diffuse.end());

        auto baseColor = loadMaterialTextures(
            material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
        textures.insert(textures.end(), baseColor.begin(), baseColor.end());
    }

    return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<MeshTexture> Model::loadMaterialTextures(
    aiMaterial* mat, aiTextureType type,
    const std::string& typeName, const aiScene* scene)
{
    std::vector<MeshTexture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texPath = str.C_Str();

        bool alreadyLoaded = false;
        for (const auto& loaded : m_loadedTextures) {
            if (loaded.path == texPath) {
                textures.push_back(loaded);
                alreadyLoaded = true;
                break;
            }
        }
        if (alreadyLoaded) continue;

        MeshTexture tex;
        tex.type = typeName;
        tex.path = texPath;

        // GLB
        const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.c_str());
        if (embedded) {
            tex.id = loadEmbeddedTexture(embedded);
        } else {
            std::string fullPath = m_directory + "/" + texPath;
            tex.id = loadTextureFromFile(fullPath);
        }

        if (tex.id != 0) {
            m_loadedTextures.push_back(tex);
            textures.push_back(tex);
        }
    }

    return textures;
}

unsigned int Model::loadEmbeddedTexture(const aiTexture* tex)
{
    int width, height, channels;
    unsigned char* data = nullptr;

    if (tex->mHeight == 0) {
        // Сжатая текстура (PNG/JPG) — декодируем из памяти
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(tex->pcData),
            static_cast<int>(tex->mWidth),
            &width, &height, &channels, 0);
    } else {
        // Несжатая ARGB8888 — конвертируем в RGBA
        width  = static_cast<int>(tex->mWidth);
        height = static_cast<int>(tex->mHeight);
        channels = 4;
        data = reinterpret_cast<unsigned char*>(tex->pcData);
    }

    if (!data) {
        std::cerr << "Failed to decode embedded texture" << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Освобождаем только если stbi выделял память
    if (tex->mHeight == 0)
        stbi_image_free(data);

    return textureID;
}

unsigned int Model::loadTextureFromFile(const std::string& filepath)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}
