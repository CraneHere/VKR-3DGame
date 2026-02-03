#include <iostream>
#include <vector>
#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "core/Constants.h"
#include "core/PhysicsSystem.h"
#include "game/GameObject.h"
#include "game/Player.h"
#include "game/ODMGear.h"
#include "graphics/Primitives.h"

const char* BASIC_VERTEX_SHADER = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragNormal;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    FragNormal = mat3(transpose(inverse(uModel))) * aNormal;
}
)";

const char* BASIC_FRAGMENT_SHADER = R"(
#version 450 core
in vec3 FragNormal;
out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uLightDir;

void main() {
    vec3 normal = normalize(FragNormal);
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(normal, -lightDir), 0.0);
    float ambient = 0.3;
    vec3 result = uColor * (ambient + diff * 0.7);
    FragColor = vec4(result, 1.0);
}
)";

const char* ROPE_VERTEX_SHADER = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
uniform mat4 uVP;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
}
)";

const char* ROPE_FRAGMENT_SHADER = R"(
#version 450 core
out vec4 FragColor;
uniform vec3 uRopeColor;
void main() {
    FragColor = vec4(uRopeColor, 1.0);
}
)";

unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) { SDL_Quit(); return -1; }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) { SDL_DestroyWindow(window); SDL_Quit(); return -1; }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_GL_DeleteContext(glContext); SDL_DestroyWindow(window); SDL_Quit(); return -1;
    }

    SDL_GL_SetSwapInterval(1);
    
    int screenWidth, screenHeight;
    SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.Alpha = 0.9f;
    
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 450");

    // Инициализация PhysX
    PhysicsSystem physicsSystem;
    if (!physicsSystem.initialize()) {
        std::cerr << "Failed to initialize PhysX!" << std::endl;
        return -1;
    }

    // Простые шейдеры
    unsigned int basicShader = createShaderProgram(BASIC_VERTEX_SHADER, BASIC_FRAGMENT_SHADER);
    unsigned int ropeShader = createShaderProgram(ROPE_VERTEX_SHADER, ROPE_FRAGMENT_SHADER);
    
    // Uniform locations
    int basicModelLoc = glGetUniformLocation(basicShader, "uModel");
    int basicViewLoc = glGetUniformLocation(basicShader, "uView");
    int basicProjLoc = glGetUniformLocation(basicShader, "uProjection");
    int basicColorLoc = glGetUniformLocation(basicShader, "uColor");
    int basicLightDirLoc = glGetUniformLocation(basicShader, "uLightDir");
    
    int ropeVPLoc = glGetUniformLocation(ropeShader, "uVP");
    int ropeColorLoc = glGetUniformLocation(ropeShader, "uRopeColor");

    // VAO куба
    unsigned int cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // VAO пола
    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FLOOR_VERTICES), FLOOR_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(FLOOR_INDICES), FLOOR_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, FLOOR_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, FLOOR_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // VAO тросов
    unsigned int ropeVAO, ropeVBO;
    glGenVertexArrays(1, &ropeVAO);
    glGenBuffers(1, &ropeVBO);
    glBindVertexArray(ropeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ropeVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * (ROPE_SEGMENTS + 1) * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Игровые объекты
    std::vector<GameObject> objects;
    objects.reserve(50);
    
    physicsSystem.createFloor();
    
    auto createBuilding = [&](glm::vec3 pos, glm::vec3 size, glm::vec3 color) {
        PxRigidStatic* actor = physicsSystem.createStaticBox(pos, size * 0.5f);
        GameObject obj;
        obj.actor = actor;
        obj.size = size;
        obj.color = color;
        obj.isDynamic = false;
        obj.isGrappleable = true;
        objects.push_back(obj);
        actor->userData = &objects.back();
    };
   
    createBuilding(glm::vec3(0.0f, 2.5f, 0.0f), glm::vec3(10.0f, 5.0f, 10.0f), glm::vec3(0.4f, 0.35f, 0.3f));

    createBuilding(glm::vec3(-25.0f, 10.0f, 20.0f), glm::vec3(8.0f, 20.0f, 8.0f), glm::vec3(0.45f, 0.4f, 0.35f));
    createBuilding(glm::vec3(25.0f, 15.0f, 20.0f), glm::vec3(8.0f, 30.0f, 8.0f), glm::vec3(0.48f, 0.43f, 0.38f));
    createBuilding(glm::vec3(0.0f, 20.0f, 40.0f), glm::vec3(10.0f, 40.0f, 10.0f), glm::vec3(0.5f, 0.5f, 0.55f));
    createBuilding(glm::vec3(-20.0f, 8.0f, 0.0f), glm::vec3(6.0f, 16.0f, 6.0f), glm::vec3(0.46f, 0.41f, 0.36f));
    createBuilding(glm::vec3(20.0f, 10.0f, 0.0f), glm::vec3(6.0f, 20.0f, 6.0f), glm::vec3(0.49f, 0.44f, 0.39f));

    createBuilding(glm::vec3(0.0f, 10.0f, 60.0f), glm::vec3(50.0f, 20.0f, 5.0f), glm::vec3(0.35f, 0.32f, 0.28f));
    createBuilding(glm::vec3(-45.0f, 10.0f, 30.0f), glm::vec3(5.0f, 20.0f, 30.0f), glm::vec3(0.35f, 0.32f, 0.28f));
    createBuilding(glm::vec3(45.0f, 10.0f, 30.0f), glm::vec3(5.0f, 20.0f, 30.0f), glm::vec3(0.35f, 0.32f, 0.28f));

    // Игрок
    Player player;
    PxController* playerController = physicsSystem.createCharacterController(
        glm::vec3(0.0f, 6.0f, -10.0f), PLAYER_HEIGHT, PLAYER_RADIUS
    );
    
    if (!playerController) {
        std::cerr << "Failed to create player controller!" << std::endl;
        return -1;
    }
    
    player.setController(playerController);
    
    SDL_SetRelativeMouseMode(SDL_TRUE);
    bool mouseCaptured = true;

    // Направление света
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));

    // Главный цикл
    bool running = true;
    SDL_Event event;
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;
    float deltaTime = 0.0f;
    float physicsAccumulator = 0.0f;

    while (running)
    {
        lastTime = currentTime;
        currentTime = SDL_GetPerformanceCounter();
        deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // События
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            if (event.type == SDL_QUIT) running = false;
            
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                if (mouseCaptured) { SDL_SetRelativeMouseMode(SDL_FALSE); mouseCaptured = false; }
                else running = false;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN && !mouseCaptured && !io.WantCaptureMouse) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                mouseCaptured = true;
            }
            
            // УПМ тросы
            if (event.type == SDL_MOUSEBUTTONDOWN && mouseCaptured && !io.WantCaptureMouse) {
                glm::vec3 forward = player.getCameraForward();
                glm::vec3 right = player.getRight();
                
                if (event.button.button == SDL_BUTTON_LEFT) {
                    player.odmGear.fireLeftGrapple(physicsSystem.scene, player.position, forward, right);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    player.odmGear.fireRightGrapple(physicsSystem.scene, player.position, forward, right);
                }
            }
            
            if (event.type == SDL_MOUSEBUTTONUP && mouseCaptured) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    player.odmGear.releaseLeftGrapple();
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    player.odmGear.releaseRightGrapple();
                }
            }
            
            if (event.type == SDL_MOUSEMOTION && mouseCaptured) {
                player.handleMouseMovement((float)event.motion.xrel, (float)event.motion.yrel);
            }
            
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                screenWidth = event.window.data1;
                screenHeight = event.window.data2;
                glViewport(0, 0, screenWidth, screenHeight);
            }
        }

        // Обновление игрока
        if (mouseCaptured) {
            const Uint8* keys = SDL_GetKeyboardState(nullptr);
            player.update(deltaTime, keys);
            player.odmGear.checkRopeCollisions(physicsSystem.scene);

            // Физика
            physicsAccumulator += deltaTime;
            while (physicsAccumulator >= PHYSICS_TIMESTEP) {
                physicsSystem.simulate(PHYSICS_TIMESTEP);
                physicsAccumulator -= PHYSICS_TIMESTEP;
            }
        }

        // Рендеринг
        glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        glm::vec3 camPos = player.getCameraPosition();
        glm::vec3 camFront = player.getCameraForward();
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(75.0f), (float)screenWidth / screenHeight, 0.1f, 500.0f);
        
        // Основной шейдер
        glUseProgram(basicShader);
        glUniformMatrix4fv(basicViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(basicProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(basicLightDirLoc, 1, glm::value_ptr(lightDir));
        
        // Пол
        glBindVertexArray(floorVAO);
        glm::mat4 floorModel = glm::mat4(1.0f);
        glUniformMatrix4fv(basicModelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
        glUniform3f(basicColorLoc, 0.35f, 0.4f, 0.32f);
        glDrawElements(GL_TRIANGLES, FLOOR_INDEX_COUNT, GL_UNSIGNED_INT, 0);

        // Объекты (здания)
        glBindVertexArray(cubeVAO);
        for (const auto& obj : objects) {
            glm::mat4 model = obj.getTransform();
            glUniformMatrix4fv(basicModelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(basicColorLoc, obj.color.x, obj.color.y, obj.color.z);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
        
        // Тросы
        glUseProgram(ropeShader);
        glUniformMatrix4fv(ropeVPLoc, 1, GL_FALSE, glm::value_ptr(proj * view));
        
        glBindVertexArray(ropeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ropeVBO);
        
        auto generateRopePoints = [](const GrappleHook& grapple, std::vector<float>& vertices) {
            if (!grapple.active) return;
            
            glm::vec3 start = grapple.visualStartPoint;
            glm::vec3 end = grapple.attachPoint;
            glm::vec3 dir = end - start;
            float ropeLen = glm::length(dir);
            
            for (int i = 0; i <= ROPE_SEGMENTS; i++) {
                float t = (float)i / ROPE_SEGMENTS;
                glm::vec3 point = start + dir * t;
                float sag = 0.5f * sin(t * 3.14159f) * (ropeLen * 0.02f);
                point.y -= sag;
                
                vertices.push_back(point.x);
                vertices.push_back(point.y);
                vertices.push_back(point.z);
            }
        };
        
        if (player.odmGear.leftGrapple.active) {
            std::vector<float> leftRopeVerts;
            generateRopePoints(player.odmGear.leftGrapple, leftRopeVerts);
            glBufferSubData(GL_ARRAY_BUFFER, 0, leftRopeVerts.size() * sizeof(float), leftRopeVerts.data());
            glUniform3f(ropeColorLoc, 0.1f, 0.1f, 0.1f);
            glLineWidth(3.0f);
            glDrawArrays(GL_LINE_STRIP, 0, ROPE_SEGMENTS + 1);
        }
        
        if (player.odmGear.rightGrapple.active) {
            std::vector<float> rightRopeVerts;
            generateRopePoints(player.odmGear.rightGrapple, rightRopeVerts);
            size_t offset = (ROPE_SEGMENTS + 1) * 3 * sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, rightRopeVerts.size() * sizeof(float), rightRopeVerts.data());
            glUniform3f(ropeColorLoc, 0.1f, 0.1f, 0.1f);
            glLineWidth(3.0f);
            glDrawArrays(GL_LINE_STRIP, ROPE_SEGMENTS + 1, ROPE_SEGMENTS + 1);
        }
        
        // Точки крепления тросов
        glUseProgram(basicShader);
        glBindVertexArray(cubeVAO);
        
        if (player.odmGear.leftGrapple.active) {
            glm::vec3 anchorPos = player.odmGear.leftGrapple.attachPoint;
            glm::mat4 anchorModel = glm::translate(glm::mat4(1.0f), anchorPos);
            anchorModel = glm::scale(anchorModel, glm::vec3(0.3f));
            glUniformMatrix4fv(basicModelLoc, 1, GL_FALSE, glm::value_ptr(anchorModel));
            glUniform3f(basicColorLoc, 1.0f, 0.3f, 0.3f);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
        
        if (player.odmGear.rightGrapple.active) {
            glm::vec3 anchorPos = player.odmGear.rightGrapple.attachPoint;
            glm::mat4 anchorModel = glm::translate(glm::mat4(1.0f), anchorPos);
            anchorModel = glm::scale(anchorModel, glm::vec3(0.3f));
            glUniformMatrix4fv(basicModelLoc, 1, GL_FALSE, glm::value_ptr(anchorModel));
            glUniform3f(basicColorLoc, 0.3f, 0.3f, 1.0f);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
        
        glBindVertexArray(0);

        // ImGui интерфейс
        ImGui::SetNextWindowPos(ImVec2((float)screenWidth - 320, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(310, 350), ImGuiCond_Always);
        
        ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        if (!mouseCaptured) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "PAUSED - Click to resume");
            ImGui::Separator();
        }
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PLAYER");
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", player.position.x, player.position.y, player.position.z);
        ImGui::Text("Speed: %.1f m/s", player.realSpeed);
        ImGui::Text("Grounded: %s", player.isGrounded ? "YES" : "NO");
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "ODM GEAR (Ground Movement)");
        
        ImGui::Text("Active: %s", player.odmGear.isActive ? "YES" : "NO");
        
        if (player.odmGear.leftGrapple.active) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Left: ACTIVE");
            ImGui::Text("  Length: %.1fm", player.odmGear.leftGrapple.length);
        } else {
            ImGui::Text("Left: ---");
        }
        
        if (player.odmGear.rightGrapple.active) {
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 1.0f, 1.0f), "Right: ACTIVE");
            ImGui::Text("  Length: %.1fm", player.odmGear.rightGrapple.length);
        } else {
            ImGui::Text("Right: ---");
        }
        
        if (player.odmGear.leftGrapple.active && player.odmGear.rightGrapple.active) {
            float separation = glm::length(
                player.odmGear.leftGrapple.attachPoint - 
                player.odmGear.rightGrapple.attachPoint
            );
            ImVec4 sepColor = separation > GRAPPLE_BREAK_DISTANCE 
                ? ImVec4(1.0f, 0.3f, 0.0f, 1.0f)
                : ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
            ImGui::TextColored(sepColor, "Separation: %.1fm", separation);
        }
        
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Controls:");
        ImGui::Text("WASD - Move");
        ImGui::Text("Space - Jump");
        ImGui::Text("LMB/RMB - Fire grapples");
        ImGui::Text("Shift + WASD - Reel movement");
        ImGui::Text("ESC - Pause/Quit");
        
        ImGui::End();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // Очистка
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    glDeleteProgram(basicShader);
    glDeleteProgram(ropeShader);
    
    physicsSystem.shutdown();
    
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteVertexArrays(1, &ropeVAO);
    glDeleteBuffers(1, &ropeVBO);
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
