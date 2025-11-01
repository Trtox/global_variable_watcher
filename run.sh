#!/bin/bash
set -e

BUILD_DIR="build"
EXECUTABLE="$BUILD_DIR/gwatch"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Building project..."
    cmake -B "$BUILD_DIR" -S .
    cmake --build "$BUILD_DIR" -j "$(nproc)"
else
    echo "Found existing build: $EXECUTABLE"
fi

"$EXECUTABLE" "$@"


