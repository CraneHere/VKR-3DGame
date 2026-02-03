#pragma once

class Camera
{
public:
    Camera();
    ~Camera() = default;

    void Update(float deltaTime);
    
    void ProcessKeyboard(float deltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset);
    void ProcessMouseScroll(float yOffset);
    
    const float* GetViewMatrix() const;
    const float* GetProjectionMatrix() const;
    
    void SetPosition(float x, float y, float z);
    void GetPosition(float& x, float& y, float& z) const;
    void GetFront(float& x, float& y, float& z) const;
    
    void SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
    
    void SetSpeed(float speed) { m_Speed = speed; }
    void SetSensitivity(float sensitivity) { m_Sensitivity = sensitivity; }

private:
    void UpdateCameraVectors();

private:
    float m_Position[3] = {0.0f, 2.0f, 5.0f};
    float m_Front[3] = {0.0f, 0.0f, -1.0f};
    float m_Up[3] = {0.0f, 1.0f, 0.0f};
    float m_Right[3] = {1.0f, 0.0f, 0.0f};
    float m_WorldUp[3] = {0.0f, 1.0f, 0.0f};
    
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    
    float m_Speed = 5.0f;
    float m_Sensitivity = 0.1f;
    float m_Zoom = 45.0f;
    
    float m_ViewMatrix[16];
    float m_ProjectionMatrix[16];
};
