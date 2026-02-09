build:
    cmake -B build
    cmake --build build

run *ARGS: build
    sh scripts/run.sh {{ARGS}}

winter: build
    sh scripts/run.sh --winter

commit:
    ./jj-commit.sh
