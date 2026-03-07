#include <iostream>
#include <vector>
#include <cfloat>
#include <filesystem>
#include <algorithm>
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
#include "graphics/ShaderSources.h"
#include "graphics/Primitives.h"
#include "graphics/Renderer.h"
#include "graphics/ShadowMap.h"
#include "graphics/Shader.h"
#include "graphics/Skysphere.h"
#include "graphics/Model.h"
#include "core/ModelSerializer.h"

int main(int argc, char* argv[])
{
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
    bool vsyncEnabled = true;
    
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
    
    // Шейдеры
    Shader mainShader, shadowShader, ropeShader;
    
    // Основной шейдер
    if (!mainShader.LoadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag")) {
        std::cerr << "Failed to load main atmospheric shader!" << std::endl;
        return -1;
    }
    
    // Шейдер теней
    if (!shadowShader.LoadFromFiles("assets/shaders/shadow.vert", "assets/shaders/shadow.frag")) {
        std::cerr << "Failed to load shadow shader!" << std::endl;
        return -1;
    }
    
    // Шейдер тросов
    if (!ropeShader.LoadFromSource(ROPE_VERTEX_SHADER_SOURCE, ROPE_FRAGMENT_SHADER_SOURCE)) {
        std::cerr << "Failed to load rope shader!" << std::endl;
        return -1;
    }
    
    // Shadow Map
    ShadowMap shadowMap;
    shadowMap.Initialize(8192, 8192);
    
    // Skysphere (небо)
    Skysphere skysphere;
    skysphere.Initialize(64, 32);
    
    // Настройки освещения
    DirectionalLight sunLight;
    sunLight.direction = glm::normalize(glm::vec3(-0.5f, -0.3f, -0.7f));
    sunLight.color = glm::vec3(1.0f, 0.7f, 0.4f);
    sunLight.intensity = 1.2f;
    
    AmbientLight ambientLight;
    ambientLight.skyColor = glm::vec3(0.6f, 0.5f, 0.5f);
    ambientLight.groundColor = glm::vec3(0.3f, 0.25f, 0.2f);
    ambientLight.intensity = 0.35f;
    
    FogSettings fog;
    fog.color = glm::vec3(0.9f, 0.6f, 0.4f);
    fog.density = 0.008f;
    fog.heightFalloff = 0.02f;
    fog.enabled = true;
    
    float sceneRadius = 150.0f;
    bool shadowsEnabled = true;
    
    // VAO куба (с нормалями)
    unsigned int cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, CUBE_STRIDE * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // VAO пола (с нормалями)
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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, FLOOR_STRIDE * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // VAO стены (с нормалями)
    unsigned int wallVAO, wallVBO, wallEBO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glGenBuffers(1, &wallEBO);
    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(WALL_VERTICES), WALL_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(WALL_INDICES), WALL_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, WALL_STRIDE * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, WALL_STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, WALL_STRIDE * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
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
    
    auto createDynamicBox = [&](glm::vec3 pos, glm::vec3 size, glm::vec3 color) {
        PxRigidDynamic* actor = physicsSystem.createDynamicBox(pos, size * 0.5f);
        GameObject obj;
        obj.actor = actor;
        obj.size = size;
        obj.color = color;
        obj.isDynamic = true;
        obj.isGrappleable = false;
        objects.push_back(obj);
        actor->userData = &objects.back();
    };

    struct ModelEntry {
        Model model;
        std::string name;
        std::string path;
        PxTriangleMesh* collisionMesh = nullptr;
        float defaultScale = 0.02f;
        bool grappleable = true;
    };
    std::vector<ModelEntry> modelRegistry;

    {
        const std::string modelsDir = "assets/models";
        for (const auto& entry : std::filesystem::directory_iterator(modelsDir)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".glb" && ext != ".gltf" && ext != ".fbx" && ext != ".obj") continue;

            ModelEntry me;
            me.path = entry.path().string();
            me.name = entry.path().filename().string();
            if (me.model.LoadFromFile(me.path)) {
                me.collisionMesh = physicsSystem.cookTriangleMesh(
                    me.model.GetCollisionVertices(), me.model.GetCollisionIndices());
                modelRegistry.push_back(std::move(me));
            }
        }
        std::sort(modelRegistry.begin(), modelRegistry.end(),
            [](const ModelEntry& a, const ModelEntry& b) { return a.name < b.name; });
    }

    struct ModelInstance {
        int modelIndex;
        glm::vec3 position;
        float scale;
        float rotationY;
        PxRigidStatic* physicsActor = nullptr;
    };
    std::vector<ModelInstance> placedModels;
    const std::string scenePath = "assets/houses.json";

    GameObject grappleMarker;
    grappleMarker.isGrappleable = true;

    auto findModelIndex = [&](const std::string& name) -> int {
        for (int i = 0; i < (int)modelRegistry.size(); i++)
            if (modelRegistry[i].name == name) return i;
        return -1;
    };

    {
        auto loaded = ModelSerializer::Load(scenePath);
        for (const auto& d : loaded) {
            int idx = findModelIndex(d.modelName);
            if (idx < 0) {
                std::cerr << "Model not found in registry: " << d.modelName << std::endl;
                continue;
            }
            ModelInstance mi;
            mi.modelIndex = idx;
            mi.position = d.position;
            mi.scale = d.scale;
            mi.rotationY = d.rotationY;
            if (modelRegistry[idx].collisionMesh) {
                mi.physicsActor = physicsSystem.createStaticTriangleMesh(
                    modelRegistry[idx].collisionMesh, mi.position, mi.scale, mi.rotationY);
                if (modelRegistry[idx].grappleable)
                    mi.physicsActor->userData = &grappleMarker;
            }
            placedModels.push_back(mi);
        }
    }

    bool editorMode = false;
    bool editorTogglePressed = false;
    int selectedModelIndex = 0;
    float ghostRotationY = 0.0f;
    float ghostScale = modelRegistry.empty() ? 0.02f : modelRegistry[0].defaultScale;
    glm::vec3 ghostPosition(0.0f);
    bool ghostValid = false;

    Player player;
    PxController* playerController = physicsSystem.createCharacterController(
        glm::vec3(0.0f, 6.0f, 0.0f), PLAYER_HEIGHT, PLAYER_RADIUS
    );
    
    if (!playerController) {
        std::cerr << "Failed to create player controller!" << std::endl;
        return -1;
    }
    
    player.setController(playerController);
    
    SDL_SetRelativeMouseMode(SDL_TRUE);
    bool mouseCaptured = true;

    // ГЛАВНЫЙ ЦИКЛ
    
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
            
            if (event.type == SDL_MOUSEBUTTONDOWN && mouseCaptured && !io.WantCaptureMouse && !editorMode) {
                glm::vec3 forward = player.getCameraForward();
                glm::vec3 right = player.getRight();
                
                if (event.button.button == SDL_BUTTON_LEFT) {
                    player.odmGear.fireLeftGrapple(physicsSystem.scene, player.position, forward, right);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    player.odmGear.fireRightGrapple(physicsSystem.scene, player.position, forward, right);
                }
            }
            
            if (event.type == SDL_MOUSEBUTTONUP && mouseCaptured && !editorMode) {
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
            
            // Редактор: колесико мыши — поворот
            if (event.type == SDL_MOUSEWHEEL && editorMode && mouseCaptured) {
                ghostRotationY += event.wheel.y * 15.0f;
                if (ghostRotationY < 0.0f) ghostRotationY += 360.0f;
                if (ghostRotationY >= 360.0f) ghostRotationY -= 360.0f;
            }

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                screenWidth = event.window.data1;
                screenHeight = event.window.data2;
                glViewport(0, 0, screenWidth, screenHeight);
            }
        }

        // Tab — переключение режима редактора
        {
            const Uint8* keys = SDL_GetKeyboardState(nullptr);
            if (keys[SDL_SCANCODE_TAB] && !editorTogglePressed) {
                editorMode = !editorMode;
                editorTogglePressed = true;
                std::cout << "Editor mode: " << (editorMode ? "ON" : "OFF") << std::endl;
            }
            if (!keys[SDL_SCANCODE_TAB]) editorTogglePressed = false;

            {
                static bool savePressed = false;
                bool saveDown = editorMode && keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_S];
                if (saveDown && !savePressed) {
                    std::vector<ModelInstanceData> data;
                    data.reserve(placedModels.size());
                    for (const auto& mi : placedModels) {
                        ModelInstanceData d;
                        d.modelName = modelRegistry[mi.modelIndex].name;
                        d.position = mi.position;
                        d.scale = mi.scale;
                        d.rotationY = mi.rotationY;
                        data.push_back(d);
                    }
                    ModelSerializer::Save(scenePath, data);
                }
                savePressed = saveDown;
            }

            {
                static bool undoPressed = false;
                bool undoDown = editorMode && keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_Z];
                if (undoDown && !undoPressed && !placedModels.empty()) {
                    if (placedModels.back().physicsActor) {
                        physicsSystem.scene->removeActor(*placedModels.back().physicsActor);
                        placedModels.back().physicsActor->release();
                    }
                    placedModels.pop_back();
                    std::cout << "Undo: removed last instance (" << placedModels.size() << " remaining)" << std::endl;
                }
                undoPressed = undoDown;
            }

            {
                static bool deletePressed = false;
                bool delDown = editorMode && keys[SDL_SCANCODE_DELETE];
                if (delDown && !deletePressed && ghostValid && !placedModels.empty()) {
                    float minDist = FLT_MAX;
                    int closest = -1;
                    for (int i = 0; i < (int)placedModels.size(); i++) {
                        float d = glm::length(placedModels[i].position - ghostPosition);
                        if (d < minDist) { minDist = d; closest = i; }
                    }
                    if (closest >= 0 && minDist < 30.0f) {
                        if (placedModels[closest].physicsActor) {
                            physicsSystem.scene->removeActor(*placedModels[closest].physicsActor);
                            placedModels[closest].physicsActor->release();
                        }
                        placedModels.erase(placedModels.begin() + closest);
                        std::cout << "Deleted instance (" << placedModels.size() << " remaining)" << std::endl;
                    }
                }
                deletePressed = delDown;
            }
        }

        // Редактор: вычислить позицию
        ghostValid = false;
        if (editorMode && mouseCaptured) {
            glm::vec3 camP = player.getCameraPosition();
            glm::vec3 camF = player.getCameraForward();
            if (camF.y < -0.01f) {
                float t = -camP.y / camF.y;
                if (t > 0.0f && t < 500.0f) {
                    ghostPosition = camP + camF * t;
                    ghostValid = true;
                }
            }
        }

        if (editorMode && mouseCaptured && !modelRegistry.empty()) {
            Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
            static bool placementClick = false;
            if ((mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) && !placementClick && ghostValid) {
                ModelInstance mi;
                mi.modelIndex = selectedModelIndex;
                mi.position = ghostPosition;
                mi.scale = ghostScale;
                mi.rotationY = ghostRotationY;
                auto& entry = modelRegistry[selectedModelIndex];
                if (entry.collisionMesh) {
                    mi.physicsActor = physicsSystem.createStaticTriangleMesh(
                        entry.collisionMesh, mi.position, mi.scale, mi.rotationY);
                    if (entry.grappleable)
                        mi.physicsActor->userData = &grappleMarker;
                }
                placedModels.push_back(mi);
                std::cout << "Placed " << entry.name << " at ("
                          << ghostPosition.x << ", " << ghostPosition.y << ", " << ghostPosition.z
                          << ") scale=" << ghostScale << " rot=" << ghostRotationY
                          << " (" << placedModels.size() << " total)" << std::endl;
                placementClick = true;
            }
            if (!(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))) placementClick = false;
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

        // РЕНДЕРИНГ
        
        glm::vec3 camPos = player.getCameraPosition();
        glm::vec3 camFront = player.getCameraForward();
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(75.0f), (float)screenWidth / screenHeight, 0.5f, 5000.0f);
        
        glm::vec3 shadowCenter = camPos;
        shadowCenter.y = 0.0f;
        glm::mat4 lightSpaceMatrix = shadowMap.CalculateLightSpaceMatrix(
            sunLight.direction, shadowCenter, sceneRadius
        );
        
        // Матрица модели пола
        glm::mat4 floorModel = glm::mat4(1.0f);
        
        // Теневая область

        if (shadowsEnabled)
        {
            shadowMap.Bind();
            shadowMap.Clear();
            glCullFace(GL_FRONT); // Без shadow acne
            
            shadowShader.Use();
            shadowShader.SetMat4("uLightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
            
            // Рендерим пол в shadow map
            glBindVertexArray(floorVAO);
            shadowShader.SetMat4("uModel", glm::value_ptr(floorModel));
            glDrawElements(GL_TRIANGLES, FLOOR_INDEX_COUNT, GL_UNSIGNED_INT, 0);
            
            // Рендерим все здания в shadow map
            glBindVertexArray(cubeVAO);
            for (const auto& obj : objects) {
                glm::mat4 model = obj.getTransform();
                shadowShader.SetMat4("uModel", glm::value_ptr(model));
                glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
            }

            for (const auto& mi : placedModels) {
                glm::mat4 hm = glm::translate(glm::mat4(1.0f), mi.position);
                hm = glm::rotate(hm, glm::radians(mi.rotationY), glm::vec3(0, 1, 0));
                hm = glm::scale(hm, glm::vec3(mi.scale));
                shadowShader.SetMat4("uModel", glm::value_ptr(hm));
                modelRegistry[mi.modelIndex].model.Draw(shadowShader.GetID());
            }

            glCullFace(GL_BACK);
            shadowMap.Unbind();
        }
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        // Небо
        skysphere.SetSunDirection(sunLight.direction);
        skysphere.Render(view, proj);
        
        mainShader.Use();
        
        // Матрицы камеры
        mainShader.SetMat4("uView", glm::value_ptr(view));
        mainShader.SetMat4("uProjection", glm::value_ptr(proj));
        mainShader.SetMat4("uLightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
        mainShader.SetVec3("uViewPos", camPos.x, camPos.y, camPos.z);
        
        // Directional Light
        mainShader.SetVec3("uLightDir", sunLight.direction.x, sunLight.direction.y, sunLight.direction.z);
        mainShader.SetVec3("uLightColor", sunLight.color.r, sunLight.color.g, sunLight.color.b);
        mainShader.SetFloat("uLightIntensity", sunLight.intensity);
        
        // Hemisphere Ambient
        mainShader.SetVec3("uSkyColor", ambientLight.skyColor.r, ambientLight.skyColor.g, ambientLight.skyColor.b);
        mainShader.SetVec3("uGroundColor", ambientLight.groundColor.r, ambientLight.groundColor.g, ambientLight.groundColor.b);
        mainShader.SetFloat("uAmbientIntensity", ambientLight.intensity);
        
        // Туман
        mainShader.SetVec3("uFogColor", fog.color.r, fog.color.g, fog.color.b);
        mainShader.SetFloat("uFogDensity", fog.density);
        mainShader.SetFloat("uFogHeightFalloff", fog.heightFalloff);
        mainShader.SetBool("uFogEnabled", fog.enabled);
        
        // Shadow Map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap.GetDepthTexture());
        mainShader.SetInt("uShadowMap", 1);
        mainShader.SetBool("uShadowsEnabled", shadowsEnabled);
        mainShader.SetFloat("uAlpha", 1.0f);
        mainShader.SetBool("uUseTexture", false);
        
        // Пол
        glBindVertexArray(floorVAO);
        mainShader.SetMat4("uModel", glm::value_ptr(floorModel));
        mainShader.SetVec3("uObjectColor", 0.5f, 0.5f, 0.5f);
        glDrawElements(GL_TRIANGLES, FLOOR_INDEX_COUNT, GL_UNSIGNED_INT, 0);

        // Объекты (здания)
        glBindVertexArray(cubeVAO);
        for (const auto& obj : objects) {
            glm::mat4 model = obj.getTransform();
            mainShader.SetMat4("uModel", glm::value_ptr(model));
            mainShader.SetVec3("uObjectColor", obj.color.x, obj.color.y, obj.color.z);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }

        glDisable(GL_CULL_FACE);
        for (const auto& mi : placedModels) {
            glm::mat4 hm = glm::translate(glm::mat4(1.0f), mi.position);
            hm = glm::rotate(hm, glm::radians(mi.rotationY), glm::vec3(0, 1, 0));
            hm = glm::scale(hm, glm::vec3(mi.scale));
            mainShader.SetMat4("uModel", glm::value_ptr(hm));
            mainShader.SetVec3("uObjectColor", 1.0f, 1.0f, 1.0f);
            modelRegistry[mi.modelIndex].model.Draw(mainShader.GetID());
            mainShader.SetBool("uUseTexture", false);
        }

        if (editorMode && ghostValid && !modelRegistry.empty()) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            glm::mat4 ghostMat = glm::translate(glm::mat4(1.0f), ghostPosition);
            ghostMat = glm::rotate(ghostMat, glm::radians(ghostRotationY), glm::vec3(0, 1, 0));
            ghostMat = glm::scale(ghostMat, glm::vec3(ghostScale));
            mainShader.SetMat4("uModel", glm::value_ptr(ghostMat));
            mainShader.SetVec3("uObjectColor", 0.3f, 1.0f, 0.4f);
            mainShader.SetFloat("uAlpha", 0.4f);
            mainShader.SetBool("uUseTexture", false);
            modelRegistry[selectedModelIndex].model.Draw(mainShader.GetID());

            mainShader.SetFloat("uAlpha", 1.0f);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        glEnable(GL_CULL_FACE);
        mainShader.SetBool("uUseTexture", false);

        // Тросы (простой шейдер)
        ropeShader.Use();
        int vpLoc = glGetUniformLocation(ropeShader.GetID(), "uVP");
        int ropeColorLoc = glGetUniformLocation(ropeShader.GetID(), "uRopeColor");
        glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(proj * view));
        
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
        mainShader.Use();
        glBindVertexArray(cubeVAO);
        
        if (player.odmGear.leftGrapple.active) {
            glm::vec3 anchorPos = player.odmGear.leftGrapple.attachPoint;
            glm::mat4 anchorModel = glm::translate(glm::mat4(1.0f), anchorPos);
            anchorModel = glm::scale(anchorModel, glm::vec3(0.3f));
            mainShader.SetMat4("uModel", glm::value_ptr(anchorModel));
            mainShader.SetVec3("uObjectColor", 1.0f, 0.3f, 0.3f);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
        
        if (player.odmGear.rightGrapple.active) {
            glm::vec3 anchorPos = player.odmGear.rightGrapple.attachPoint;
            glm::mat4 anchorModel = glm::translate(glm::mat4(1.0f), anchorPos);
            anchorModel = glm::scale(anchorModel, glm::vec3(0.3f));
            mainShader.SetMat4("uModel", glm::value_ptr(anchorModel));
            mainShader.SetVec3("uObjectColor", 0.3f, 0.3f, 1.0f);
            glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        }
        
        glBindVertexArray(0);
        
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 480), ImGuiCond_FirstUseEver);
        ImGui::Begin("Shader Settings");

        if (!mouseCaptured) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "PAUSED - Use settings freely");
            if (ImGui::Button("Resume Game", ImVec2(-1, 30))) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                mouseCaptured = true;
            }
            ImGui::Separator();
        }
        
        if (ImGui::CollapsingHeader("Sun Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat3("Direction", &sunLight.direction.x, -1.0f, 1.0f);
            sunLight.direction = glm::normalize(sunLight.direction);
            ImGui::ColorEdit3("Color##Sun", &sunLight.color.r);
            ImGui::SliderFloat("Intensity##Sun", &sunLight.intensity, 0.0f, 2.0f);
        }
        
        if (ImGui::CollapsingHeader("Ambient")) {
            ImGui::ColorEdit3("Sky Color", &ambientLight.skyColor.r);
            ImGui::ColorEdit3("Ground Color", &ambientLight.groundColor.r);
            ImGui::SliderFloat("Intensity##Amb", &ambientLight.intensity, 0.0f, 1.0f);
        }
        
        if (ImGui::CollapsingHeader("Sky (Skysphere)")) {
            ImGui::ColorEdit3("Horizon Color", &skysphere.GetHorizonColor().r);
            ImGui::ColorEdit3("Zenith Color", &skysphere.GetZenithColor().r);
            ImGui::ColorEdit3("Sun Glow Color", &skysphere.GetSunColor().r);
            ImGui::SliderFloat("Sun Size", &skysphere.GetSunSize(), 0.01f, 0.2f);
        }
        
        if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enabled", &fog.enabled);
            ImGui::ColorEdit3("Fog Color", &fog.color.r);
            ImGui::SliderFloat("Density", &fog.density, 0.0f, 0.05f, "%.4f");
            ImGui::SliderFloat("Height Falloff", &fog.heightFalloff, 0.0f, 0.2f, "%.3f");
        }
        
        if (ImGui::CollapsingHeader("Performance")) {
            ImGui::Checkbox("Shadows Enabled", &shadowsEnabled);
            if (ImGui::Checkbox("V-Sync", &vsyncEnabled)) {
                SDL_GL_SetSwapInterval(vsyncEnabled ? 1 : 0);
            }
            ImGui::Text("Shadow Map: 8192x8192");
            ImGui::Text("Objects: %d", (int)objects.size());
            if (!shadowsEnabled) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shadows OFF - better FPS");
            }
        }
        
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(10, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 320), ImGuiCond_FirstUseEver);
        ImGui::Begin("Model Editor");
        if (modelRegistry.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No models in assets/models/");
        } else if (editorMode) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "EDITOR MODE: ON");
            ImGui::Separator();

            if (ImGui::BeginCombo("Model", modelRegistry[selectedModelIndex].name.c_str())) {
                for (int i = 0; i < (int)modelRegistry.size(); i++) {
                    bool selected = (i == selectedModelIndex);
                    if (ImGui::Selectable(modelRegistry[i].name.c_str(), selected)) {
                        selectedModelIndex = i;
                        ghostScale = modelRegistry[i].defaultScale;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SliderFloat("Scale", &ghostScale, 0.001f, 500.0f, "%.3f");
            ImGui::SliderFloat("Rotation", &ghostRotationY, 0.0f, 360.0f);
            ImGui::Text("Placed: %d", (int)placedModels.size());
            if (ghostValid) {
                ImGui::Text("Aim: (%.1f, %.1f, %.1f)",
                    ghostPosition.x, ghostPosition.y, ghostPosition.z);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Look down to aim");
            }
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Controls:");
            ImGui::BulletText("LMB - Place model");
            ImGui::BulletText("Scroll - Rotate");
            ImGui::BulletText("Delete - Remove nearest");
            ImGui::BulletText("Ctrl+S - Save");
            ImGui::BulletText("Ctrl+Z - Undo");
            ImGui::BulletText("Tab - Exit editor");
        } else {
            ImGui::Text("Models loaded: %d", (int)modelRegistry.size());
            ImGui::Text("Placed: %d", (int)placedModels.size());
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Tab to enter editor");
        }
        ImGui::End();

        // Debug Info
        ImGui::SetNextWindowPos(ImVec2((float)screenWidth - 320, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(310, 450), ImGuiCond_Always);
        
        ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PLAYER");
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", player.position.x, player.position.y, player.position.z);
        
        ImGui::Text("Speed: %.1f m/s", player.realSpeed);
        ImGui::Text("Horizontal: %.1f m/s", player.horizontalSpeed);
        ImGui::Text("Grounded: %s", player.isGrounded ? "YES" : "NO");
        
        ImGui::Separator();
        float slideLen = glm::length(player.slideVelocity);
        ImGui::Text("SlideVel: (%.1f, %.1f, %.1f)", player.slideVelocity.x, player.slideVelocity.y, player.slideVelocity.z);
        ImGui::Text("SlideSpeed: %.1f", slideLen);
        ImGui::Text("FlyMode: %s", player.flyMode ? "YES" : "NO");
        ImGui::Text("ODM+Air: %s", (player.odmGear.isActive && !player.isGrounded) ? "YES" : "NO");
        
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "ODM GEAR");
        
        float gasPercent = player.odmGear.gasAmount / GAS_MAX_AMOUNT;
        ImVec4 gasColor = gasPercent > 0.3f ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.3f, 0.0f, 1.0f);
        ImGui::TextColored(gasColor, "Gas: %.0f%%", gasPercent * 100.0f);
        ImGui::ProgressBar(gasPercent, ImVec2(-1, 0), "");
        
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
            if (separation > GRAPPLE_BREAK_DISTANCE) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "  WARNING: Too far!");
            }
        }
        
        ImGui::End();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    for (auto& me : modelRegistry)
        me.model.Cleanup();
    shadowMap.Shutdown();
    skysphere.Shutdown();
    physicsSystem.shutdown();
    
    glDeleteVertexArrays(1, &cubeVAO);

    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteBuffers(1, &wallEBO);
    glDeleteVertexArrays(1, &ropeVAO);
    glDeleteBuffers(1, &ropeVBO);
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
