#version 450 core

// Пустой фрагментный шейдер для shadow pass
// OpenGL автоматически записывает глубину в depth buffer
// Нам не нужно ничего выводить в color buffer

void main()
{
    // gl_FragDepth автоматически заполняется OpenGL
    // Можно оставить шейдер пустым
}
