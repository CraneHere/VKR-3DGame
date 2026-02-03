#pragma once

// Окно
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* const WINDOW_TITLE = "VKR-3DGame Engine";

// Игрок
const float PLAYER_HEIGHT = 1.8f;
const float PLAYER_RADIUS = 0.35f;
const float WALK_SPEED = 6.0f;
const float JUMP_FORCE = 7.0f;
const float GRAVITY = 18.0f;

// УПМ
const float GRAPPLE_MAX_DISTANCE = 100.0f;
const float GRAPPLE_SCAN_ANGLE = 60.0f;
const float GRAPPLE_SCAN_STEP = 1.0f;
const float GRAPPLE_MIN_SEPARATION = 2.0f;
const float GROUND_REEL_SPEED = 24.0f;
const float GRAPPLE_BREAK_DISTANCE = 25.0f;  // Расстояние между тросами 

// Физика
const float PHYSICS_TIMESTEP = 1.0f / 60.0f;

// Рендеринг
const int ROPE_SEGMENTS = 20;
