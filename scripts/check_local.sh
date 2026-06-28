#!/bin/sh
set -eu
cmake -S . -B build -GNinja
cmake --build build
ctest --test-dir build --output-on-failure
