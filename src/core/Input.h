#pragma once

class Input
{
public:
    static void Update();
    
    static bool IsKeyPressed(int keycode);
    static bool IsKeyDown(int keycode);
    static bool IsKeyUp(int keycode);
    
    static bool IsMouseButtonPressed(int button);
    static float GetMouseX();
    static float GetMouseY();
    static float GetMouseDeltaX();
    static float GetMouseDeltaY();
    
    static void SetMouseCaptured(bool captured);
    static bool IsMouseCaptured();
};
