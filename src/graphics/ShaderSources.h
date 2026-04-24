#pragma once

const char* const VERTEX_SHADER_SOURCE = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* const FRAGMENT_SHADER_SOURCE = R"(
#version 450 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

const char* const ROPE_VERTEX_SHADER_SOURCE = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
uniform mat4 uVP;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
}
)";

const char* const ROPE_FRAGMENT_SHADER_SOURCE = R"(
#version 450 core
uniform vec3 uRopeColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uRopeColor, 1.0);
}
)";

// Shadow Map Shaders

const char* const SHADOW_VERTEX_SHADER_SOURCE = R"(
#version 450 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;
void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
)";

const char* const SHADOW_FRAGMENT_SHADER_SOURCE = R"(
#version 450 core
void main() {
    // Depth записывается автоматически
}
)";
