#pragma once

class Renderer
{
public:
    static void Initialize();
    static void Shutdown();
    
    static void BeginFrame();
    static void EndFrame();
    
    static void Clear();
    static void SetClearColor(float r, float g, float b, float a = 1.0f);
    static void SetViewport(int x, int y, int width, int height);
    
    static void DrawCube();
    static void DrawSphere();
    static void DrawPlane();
};
