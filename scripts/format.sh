#!/bin/bash

# Change to project root directory
cd "$(dirname "$0")/.."

echo "🎨 Formatting code..."
find src include tests -name "*.cpp" -o -name "*.hpp" 2>/dev/null | xargs clang-format -i 2>/dev/null || {
    echo "⚠️  clang-format not found. Install it to use code formatting."
    exit 1
}
echo "✅ Code formatting complete!"
