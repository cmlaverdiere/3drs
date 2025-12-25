# Default to main repo, can override with: just run workspace=foo
workspace := ""

# Build directory path based on workspace
build_dir := if workspace == "" { "build" } else { "workspaces/" + workspace + "/build" }
game_dir := if workspace == "" { "." } else { "workspaces/" + workspace }

build:
    cmake -B {{game_dir}}/build -S {{game_dir}}
    cmake --build {{game_dir}}/build

run: build
    {{game_dir}}/build/game

test: build
    {{game_dir}}/build/game --test

screenshot: build
    {{game_dir}}/build/game --screenshot
    ./last_screenshots.sh 1

commit:
    ./jj-commit.sh

# Create a new workspace for feature development
workspace-add name:
    jj workspace add workspaces/{{name}}

# Remove a workspace after merging
workspace-remove name:
    jj workspace forget {{name}}
    rm -rf workspaces/{{name}}

# List all workspaces
workspace-list:
    jj workspace list
