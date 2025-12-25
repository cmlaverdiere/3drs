build:
    cmake -B build
    cmake --build build

run *ARGS: build
    ./build/game {{ARGS}}

winter: build
    ./build/game --winter

commit:
    ./jj-commit.sh
