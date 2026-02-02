#!/bin/bash
set -e

# Change to project root directory
cd "$(dirname "$0")/.."

# Create build directory if it doesn't exist
mkdir -p build

# Configure and build
echo "🔧 Configuring project..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

echo "🔨 Building project..."
cmake --build build

echo "✅ Build complete! Run with: ./bin/main"
