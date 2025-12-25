cmake_minimum_required(VERSION 3.14)
project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# Piper TTS library (pre-built in external/piper/libpiper/install)
set(PIPER_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/piper/libpiper/install)
set(PIPER_INCLUDE_DIR ${PIPER_INSTALL_DIR}/include)
set(PIPER_LIB_DIR ${PIPER_INSTALL_DIR})
set(ONNX_LIB_DIR ${PIPER_INSTALL_DIR}/lib)

add_executable(game
    src/main.cpp
    src/types.cpp
    src/xp_system.cpp
    src/collision.cpp
    src/map.cpp
    src/save_system.cpp
    src/rendering.cpp
    src/game_systems.cpp
    src/sound_system.cpp
    src/player.cpp
    src/enemy_ai.cpp
    src/combat.cpp
    src/inventory.cpp
    src/hud.cpp
    src/game_init.cpp
    src/lighting.cpp
    src/quest_system.cpp
    src/voice_system.cpp
)

target_include_directories(game PRIVATE ${PIPER_INCLUDE_DIR})
target_link_directories(game PRIVATE ${PIPER_LIB_DIR} ${ONNX_LIB_DIR})
target_link_libraries(game raylib piper onnxruntime)

# Set RPATH so the game can find the dynamic libraries at runtime
set_target_properties(game PROPERTIES
    BUILD_RPATH "${PIPER_LIB_DIR};${ONNX_LIB_DIR}"
    INSTALL_RPATH "${PIPER_LIB_DIR};${ONNX_LIB_DIR}"
)
