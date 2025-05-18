#!/bin/bash

shopt -s xpg_echo

if [ -d "build" ]; then
    rm -rf build
fi

echo "\nBuilding Examples...\n"

EXAMPLE_NAME="libstr_example"

# -S: Source directory, -B: Build directory
cmake -S . -B build -D BUILD_EXAMPLE=ON
cmake --build build --target build_${EXAMPLE_NAME}

echo "\nBuilding Examples is done!\n"

./build/examples/${EXAMPLE_NAME}
