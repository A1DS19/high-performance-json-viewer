# Project variables
PROJECT_NAME := $(shell basename $(CURDIR))
BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
INCLUDE_DIR := include
TESTS_DIR := tests

# Build configurations
BUILD_TYPE ?= Debug
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Default target
.PHONY: all
all: build

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  build       - Build the project (default)"
	@echo "  clean       - Clean build artifacts"
	@echo "  rebuild     - Clean and build"
	@echo "  run         - Build and run the main executable"
	@echo "  debug       - Build with debug info and run with GDB"
	@echo "  release     - Build optimized release version"
	@echo "  test        - Build and run tests"
	@echo "  format      - Format code with clang-format"
	@echo "  install     - Install to system (requires sudo)"
	@echo "  uninstall   - Remove from system (requires sudo)"
	@echo "  deps        - Show build dependencies"
	@echo "  help        - Show this help message"

# Build target
.PHONY: build
build: $(BUILD_DIR)/Makefile
	@echo "Building $(PROJECT_NAME)..."
	@cmake --build $(BUILD_DIR)
	@echo "Build complete!"

# Configure CMake
$(BUILD_DIR)/Makefile:
	@echo "Configuring CMake..."
	@mkdir -p $(BUILD_DIR)
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)

# Clean target
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)/* $(BIN_DIR)/* lib/*
	@touch $(BUILD_DIR)/.gitkeep $(BIN_DIR)/.gitkeep lib/.gitkeep
	@echo "Clean complete!"

# Rebuild target
.PHONY: rebuild
rebuild: clean build

# Run target
.PHONY: run
run: build
	@echo "Running $(PROJECT_NAME)..."
	@./$(BIN_DIR)/main

# Debug target
.PHONY: debug
debug: BUILD_TYPE = Debug
debug: build
	@echo "Running $(PROJECT_NAME) with GDB..."
	@gdb -q ./$(BIN_DIR)/main

# Release build
.PHONY: release
release: BUILD_TYPE = Release
release: clean build

# Test target
.PHONY: test
test: build
	@if [ -f $(BIN_DIR)/test_main ]; then \
		echo "Running tests..."; \
		./$(BIN_DIR)/test_main; \
	else \
		echo "No tests found. Build test target first."; \
	fi

# Format target
.PHONY: format
format:
	@echo "Formatting code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) $(TESTS_DIR) -name "*.cpp" -o -name "*.hpp" 2>/dev/null | xargs clang-format -i; \
		echo "Code formatting complete!"; \
	else \
		echo "clang-format not found. Install it to use code formatting."; \
		exit 1; \
	fi

# Install target
.PHONY: install
install: release
	@echo "Installing $(PROJECT_NAME)..."
	@sudo cp $(BIN_DIR)/main /usr/local/bin/$(PROJECT_NAME)
	@sudo chmod +x /usr/local/bin/$(PROJECT_NAME)
	@echo "Installed to /usr/local/bin/$(PROJECT_NAME)"

# Uninstall target
.PHONY: uninstall
uninstall:
	@echo "Uninstalling $(PROJECT_NAME)..."
	@sudo rm -f /usr/local/bin/$(PROJECT_NAME)
	@echo "Uninstalled from /usr/local/bin/$(PROJECT_NAME)"

# Show dependencies
.PHONY: deps
deps:
	@echo "Build dependencies:"
	@echo "  - CMake 3.14+"
	@echo "  - C++20 compatible compiler"
	@echo "  - make"
	@echo ""
	@echo "Optional dependencies:"
	@echo "  - clang-format (for code formatting)"
	@echo "  - gdb (for debugging)"

# Check if tools are available
.PHONY: check
check:
	@echo "Checking build environment..."
	@command -v cmake >/dev/null 2>&1 && echo "CMake found" || echo "CMake not found"
	@command -v make >/dev/null 2>&1 && echo "Make found" || echo "Make not found"
	@command -v g++ >/dev/null 2>&1 && echo "g++ found" || echo "g++ not found"
	@command -v clang++ >/dev/null 2>&1 && echo "clang++ found" || echo "clang++ not found"
	@command -v clang-format >/dev/null 2>&1 && echo "clang-format found" || echo "clang-format not found (optional)"
	@command -v gdb >/dev/null 2>&1 && echo "gdb found" || echo "gdb not found (optional)"

# Watch for changes and rebuild
.PHONY: watch
watch:
	@if command -v fswatch >/dev/null 2>&1; then \
		echo "Watching for changes... (Ctrl+C to stop)"; \
		fswatch -o $(SRC_DIR) $(INCLUDE_DIR) | xargs -n1 -I{} make run; \
	elif command -v inotifywait >/dev/null 2>&1; then \
		echo "Watching for changes... (Ctrl+C to stop)"; \
		while true; do \
			inotifywait -q -r -e modify,create,delete $(SRC_DIR) $(INCLUDE_DIR) && \
			make run; \
		done; \
	else \
		echo "Install fswatch (macOS: brew install fswatch) or inotify-tools (Linux)"; \
	fi
