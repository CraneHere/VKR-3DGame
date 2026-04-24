#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::~Shader()
{
    if (m_ID != 0)
    {
        glDeleteProgram(m_ID);
    }
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSource, fragmentSource;
    
    try
    {
        std::ifstream vFile(vertexPath);
        std::ifstream fFile(fragmentPath);
        
        if (!vFile.is_open())
        {
            std::cerr << "Failed to open vertex shader: " << vertexPath << std::endl;
            return false;
        }
        if (!fFile.is_open())
        {
            std::cerr << "Failed to open fragment shader: " << fragmentPath << std::endl;
            return false;
        }
        
        std::stringstream vStream, fStream;
        vStream << vFile.rdbuf();
        fStream << fFile.rdbuf();
        
        vertexSource = vStream.str();
        fragmentSource = fStream.str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error reading shader files: " << e.what() << std::endl;
        return false;
    }
    
    return LoadFromSource(vertexSource, fragmentSource);
}

bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource)
{
    // Компиляция vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!CompileShader(vertexShader, vertexSource))
    {
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Компиляция fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!CompileShader(fragmentShader, fragmentSource))
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    // Линковка программы
    if (!LinkProgram(vertexShader, fragmentShader))
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

bool Shader::CompileShader(unsigned int shader, const std::string& source)
{
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
        return false;
    }
    
    return true;
}

bool Shader::LinkProgram(unsigned int vertex, unsigned int fragment)
{
    m_ID = glCreateProgram();
    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    glLinkProgram(m_ID);
    
    int success;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(m_ID, 1024, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
        m_ID = 0;
        return false;
    }
    
    return true;
}

void Shader::Use() const
{
    glUseProgram(m_ID);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), static_cast<int>(value));
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}

void Shader::SetMat4(const std::string& name, const float* value) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, value);
}