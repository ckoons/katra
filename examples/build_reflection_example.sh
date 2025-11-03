#!/bin/bash
# Build script for reflection_example.c

echo "Building reflection example..."
gcc -Wall -Wextra -std=c11 -I../include -o reflection_example reflection_example.c \
    -L../build -lkatra_utils -lsqlite3 -lpthread -lm

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Run with: ./reflection_example"
else
    echo "✗ Build failed"
    exit 1
fi
