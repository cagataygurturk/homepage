# Homepage Server Makefile
# Project configuration
PROJECT_NAME := homepage
BUILD_DIR := build
BUILD_TYPE := Release
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# Default target
.DEFAULT_GOAL := all

# Configure and build the project
.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@echo "Configuring $(PROJECT_NAME)..."
	@cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS)
	@echo "Building $(PROJECT_NAME)..."
	@cd $(BUILD_DIR) && $(MAKE) $(PROJECT_NAME) -j$$(nproc 2>/dev/null || echo 4)

# All target (configure + build)
.PHONY: all
all: build

# Debug build
.PHONY: debug
debug: BUILD_TYPE := Debug
debug: CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug
debug: build

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf cmake-build-*

# Install (requires sudo)
.PHONY: install
install: build
	@echo "Installing $(PROJECT_NAME)..."
	@cd $(BUILD_DIR) && sudo $(MAKE) install

# Run tests (if available)
.PHONY: test
test: build
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest --verbose

# Run the server
.PHONY: run
run: build
	@echo "Starting $(PROJECT_NAME) server..."
	@./$(BUILD_DIR)/$(PROJECT_NAME)

# Run with custom port
.PHONY: run-port
run-port: build
	@echo "Starting $(PROJECT_NAME) server on port $(PORT)..."
	@PORT=$(PORT) ./$(BUILD_DIR)/$(PROJECT_NAME)

# Docker targets
.PHONY: docker-build
docker-build:
	@echo "Building Docker image..."
	@docker build -t $(PROJECT_NAME) .

.PHONY: docker-run
docker-run: docker-build
	@echo "Running Docker container..."
	@docker run -p 8080:8080 $(PROJECT_NAME)

# Development helpers
.PHONY: format
format:
	@echo "Formatting code..."
	@find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

.PHONY: lint
lint:
	@echo "Running linter..."
	@./scripts/run-clang-tidy.sh

# Show help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all          - Configure and build (default)"
	@echo "  build        - Build the project"
	@echo "  debug        - Build in debug mode"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install the project"
	@echo "  test         - Run tests"
	@echo "  run          - Run the server"
	@echo "  run-port     - Run with custom port (make run-port PORT=8081)"
	@echo "  docker-build - Build Docker image"
	@echo "  docker-run   - Build and run Docker container"
	@echo "  format       - Format code with clang-format"
	@echo "  lint         - Run clang-tidy linter"
	@echo "  help         - Show this help"

# Prevent make from trying to build files with these names
.PHONY: configure build all debug clean install test run run-port docker-build docker-run format lint help