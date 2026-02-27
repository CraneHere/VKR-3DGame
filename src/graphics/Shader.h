#pragma once

#include <string>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    
    void Use() const;
    
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;
    void SetMat4(const std::string& name, const float* value) const;

    unsigned int GetID() const { return m_ID; }

private:
    unsigned int m_ID = 0;
    
    bool CompileShader(unsigned int shader, const std::string& source);
    bool LinkProgram(unsigned int vertex, unsigned int fragment);
};
