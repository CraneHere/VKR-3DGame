#include "Input.h"
#include <SDL.h>
#include <cstring>

namespace
{
    const Uint8* s_KeyboardState = nullptr;
    Uint8 s_PreviousKeyboardState[SDL_NUM_SCANCODES] = {0};
    
    int s_MouseX = 0;
    int s_MouseY = 0;
    int s_MouseDeltaX = 0;
    int s_MouseDeltaY = 0;
    Uint32 s_MouseButtons = 0;
    
    bool s_MouseCaptured = false;
}

void Input::Update()
{
    if (s_KeyboardState)
    {
        std::memcpy(s_PreviousKeyboardState, s_KeyboardState, SDL_NUM_SCANCODES);
    }
    
    s_KeyboardState = SDL_GetKeyboardState(nullptr);
    
    int prevX = s_MouseX;
    int prevY = s_MouseY;
    s_MouseButtons = SDL_GetMouseState(&s_MouseX, &s_MouseY);
    
    if (s_MouseCaptured)
    {
        SDL_GetRelativeMouseState(&s_MouseDeltaX, &s_MouseDeltaY);
    }
    else
    {
        s_MouseDeltaX = s_MouseX - prevX;
        s_MouseDeltaY = s_MouseY - prevY;
    }
}

bool Input::IsKeyPressed(int scancode)
{
    // Нажата в этом кадре (не была нажата в прошлом)
    return s_KeyboardState && 
           s_KeyboardState[scancode] && 
           !s_PreviousKeyboardState[scancode];
}

bool Input::IsKeyDown(int scancode)
{
    // Удерживается
    return s_KeyboardState && s_KeyboardState[scancode];
}

bool Input::IsKeyUp(int scancode)
{
    // Отпущена в этом кадре
    return s_KeyboardState && 
           !s_KeyboardState[scancode] && 
           s_PreviousKeyboardState[scancode];
}

bool Input::IsMouseButtonPressed(int button)
{
    return (s_MouseButtons & SDL_BUTTON(button)) != 0;
}

float Input::GetMouseX()
{
    return static_cast<float>(s_MouseX);
}

float Input::GetMouseY()
{
    return static_cast<float>(s_MouseY);
}

float Input::GetMouseDeltaX()
{
    return static_cast<float>(s_MouseDeltaX);
}

float Input::GetMouseDeltaY()
{
    return static_cast<float>(s_MouseDeltaY);
}

void Input::SetMouseCaptured(bool captured)
{
    s_MouseCaptured = captured;
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
}

bool Input::IsMouseCaptured()
{
    return s_MouseCaptured;
}
