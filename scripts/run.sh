#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"

# Automation needs the executable's exit status and must not activate a GUI app.
for arg in "$@"; do
    case "$arg" in
        --test|--headless|--screenshot|--script)
            exec ./build/game "$@"
            ;;
    esac
done

if [ "$(uname -s)" = Darwin ]; then
    exec /usr/bin/open -n -W "$project_dir/build/3DRS.app" \
        --args --working-directory "$project_dir" "$@"
fi

exec ./build/game "$@"
