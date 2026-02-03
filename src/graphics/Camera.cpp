#include "Camera.h"
#include "core/Input.h"
#include <SDL.h>
#include <cmath>
#include <algorithm>

Camera::Camera()
{
    UpdateCameraVectors();
    
    // Инициализация матриц
    for (int i = 0; i < 16; i++)
    {
        m_ViewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        m_ProjectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

void Camera::Update(float deltaTime)
{
    ProcessKeyboard(deltaTime);
    UpdateCameraVectors();

    // View матрица
    float fx = m_Position[0] + m_Front[0];
    float fy = m_Position[1] + m_Front[1];
    float fz = m_Position[2] + m_Front[2];

    // Направление камеры
    float zx = m_Position[0] - fx;
    float zy = m_Position[1] - fy;
    float zz = m_Position[2] - fz;
    float zLen = std::sqrt(zx * zx + zy * zy + zz * zz);
    zx /= zLen; zy /= zLen; zz /= zLen;

    // Правый вектор
    float xx = m_WorldUp[1] * zz - m_WorldUp[2] * zy;
    float xy = m_WorldUp[2] * zx - m_WorldUp[0] * zz;
    float xz = m_WorldUp[0] * zy - m_WorldUp[1] * zx;
    float xLen = std::sqrt(xx * xx + xy * xy + xz * xz);
    xx /= xLen; xy /= xLen; xz /= xLen;

    // Верхний вектор
    float yx = zy * xz - zz * xy;
    float yy = zz * xx - zx * xz;
    float yz = zx * xy - zy * xx;

    m_ViewMatrix[0] = xx;  m_ViewMatrix[4] = xy;  m_ViewMatrix[8]  = xz;  m_ViewMatrix[12] = -(xx * m_Position[0] + xy * m_Position[1] + xz * m_Position[2]);
    m_ViewMatrix[1] = yx;  m_ViewMatrix[5] = yy;  m_ViewMatrix[9]  = yz;  m_ViewMatrix[13] = -(yx * m_Position[0] + yy * m_Position[1] + yz * m_Position[2]);
    m_ViewMatrix[2] = zx;  m_ViewMatrix[6] = zy;  m_ViewMatrix[10] = zz;  m_ViewMatrix[14] = -(zx * m_Position[0] + zy * m_Position[1] + zz * m_Position[2]);
    m_ViewMatrix[3] = 0;   m_ViewMatrix[7] = 0;   m_ViewMatrix[11] = 0;   m_ViewMatrix[15] = 1;
}

void Camera::ProcessKeyboard(float deltaTime)
{
    float velocity = m_Speed * deltaTime;

    if (Input::IsKeyDown(SDL_SCANCODE_W))
    {
        m_Position[0] += m_Front[0] * velocity;
        m_Position[1] += m_Front[1] * velocity;
        m_Position[2] += m_Front[2] * velocity;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_S))
    {
        m_Position[0] -= m_Front[0] * velocity;
        m_Position[1] -= m_Front[1] * velocity;
        m_Position[2] -= m_Front[2] * velocity;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_A))
    {
        m_Position[0] -= m_Right[0] * velocity;
        m_Position[1] -= m_Right[1] * velocity;
        m_Position[2] -= m_Right[2] * velocity;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_D))
    {
        m_Position[0] += m_Right[0] * velocity;
        m_Position[1] += m_Right[1] * velocity;
        m_Position[2] += m_Right[2] * velocity;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_SPACE))
    {
        m_Position[1] += velocity;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_LCTRL))
    {
        m_Position[1] -= velocity;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset)
{
    xOffset *= m_Sensitivity;
    yOffset *= m_Sensitivity;

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    // Ограничиваем pitch
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    m_Zoom -= yOffset;
    m_Zoom = std::clamp(m_Zoom, 1.0f, 90.0f);
}

const float* Camera::GetViewMatrix() const
{
    return m_ViewMatrix;
}

const float* Camera::GetProjectionMatrix() const
{
    return m_ProjectionMatrix;
}

void Camera::SetPosition(float x, float y, float z)
{
    m_Position[0] = x;
    m_Position[1] = y;
    m_Position[2] = z;
}

void Camera::GetPosition(float& x, float& y, float& z) const
{
    x = m_Position[0];
    y = m_Position[1];
    z = m_Position[2];
}

void Camera::GetFront(float& x, float& y, float& z) const
{
    x = m_Front[0];
    y = m_Front[1];
    z = m_Front[2];
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    float tanHalfFov = std::tan(fov * 0.5f * 3.14159265359f / 180.0f);
    
    for (int i = 0; i < 16; i++) m_ProjectionMatrix[i] = 0.0f;
    
    m_ProjectionMatrix[0] = 1.0f / (aspectRatio * tanHalfFov);
    m_ProjectionMatrix[5] = 1.0f / tanHalfFov;
    m_ProjectionMatrix[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m_ProjectionMatrix[11] = -1.0f;
    m_ProjectionMatrix[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
}

void Camera::UpdateCameraVectors()
{
    float yawRad = m_Yaw * 3.14159265359f / 180.0f;
    float pitchRad = m_Pitch * 3.14159265359f / 180.0f;

    m_Front[0] = std::cos(yawRad) * std::cos(pitchRad);
    m_Front[1] = std::sin(pitchRad);
    m_Front[2] = std::sin(yawRad) * std::cos(pitchRad);

    float len = std::sqrt(m_Front[0] * m_Front[0] + m_Front[1] * m_Front[1] + m_Front[2] * m_Front[2]);
    m_Front[0] /= len;
    m_Front[1] /= len;
    m_Front[2] /= len;

    m_Right[0] = m_Front[1] * m_WorldUp[2] - m_Front[2] * m_WorldUp[1];
    m_Right[1] = m_Front[2] * m_WorldUp[0] - m_Front[0] * m_WorldUp[2];
    m_Right[2] = m_Front[0] * m_WorldUp[1] - m_Front[1] * m_WorldUp[0];
    len = std::sqrt(m_Right[0] * m_Right[0] + m_Right[1] * m_Right[1] + m_Right[2] * m_Right[2]);
    m_Right[0] /= len;
    m_Right[1] /= len;
    m_Right[2] /= len;

    m_Up[0] = m_Right[1] * m_Front[2] - m_Right[2] * m_Front[1];
    m_Up[1] = m_Right[2] * m_Front[0] - m_Right[0] * m_Front[2];
    m_Up[2] = m_Right[0] * m_Front[1] - m_Right[1] * m_Front[0];
}
