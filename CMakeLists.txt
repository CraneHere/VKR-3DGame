cmake_minimum_required(VERSION 3.20)
project(VKR_3DGame VERSION 1.0.0 LANGUAGES CXX)

# C++17 стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Выходные директории
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# SDL2
find_package(SDL2 CONFIG REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# GLAD
find_package(glad CONFIG REQUIRED)

# GLM (математика)
find_package(glm CONFIG REQUIRED)

# Assimp (загрузка моделей)
find_package(assimp CONFIG REQUIRED)

# STB (текстуры)
find_package(Stb REQUIRED)

# PhysX
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

set(CORE_SOURCES
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Input.cpp
)

set(GRAPHICS_SOURCES
    src/graphics/Renderer.cpp
    src/graphics/Camera.cpp
)

set(PHYSICS_SOURCES
    # src/physics/PhysicsWorld.cpp
)

set(AUDIO_SOURCES
    # src/audio/AudioManager.cpp
)

set(GAME_SOURCES
    src/game/Game.cpp
)

# Главный исполняемый файл
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${GRAPHICS_SOURCES}
    ${PHYSICS_SOURCES}
    ${AUDIO_SOURCES}
    ${GAME_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
    ${IMGUI_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # SDL2
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    
    # OpenGL
    OpenGL::GL
    
    # GLAD
    glad::glad
    
    # GLM
    glm::glm
    
    # Assimp
    assimp::assimp
    
    # PhysX
    unofficial::omniverse-physx-sdk::sdk
)

# Копирование DLL на Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

# PhysX GPU библиотеки
if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endif()
endif()
