#!/bin/bash

# Change to project root directory
cd "$(dirname "$0")/.."

echo "🧹 Cleaning build artifacts..."
rm -rf build/* bin/* lib/*

# Recreate .gitkeep files
touch build/.gitkeep bin/.gitkeep lib/.gitkeep

echo "✅ Clean complete!"
