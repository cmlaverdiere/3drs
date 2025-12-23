build:
    cmake -B build
    cmake --build build

run: build
    ./build/game
