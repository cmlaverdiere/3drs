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
)
target_link_libraries(game raylib)
