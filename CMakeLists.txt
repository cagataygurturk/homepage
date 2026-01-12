cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(homepage)

include(cmake/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to manage dependencies
include(FetchContent)

# Fetch Drogon with minimal features
message(STATUS "Fetching Drogon from GitHub")

# Set CMake policy to allow options to be overridden
cmake_policy(SET CMP0077 NEW)

# Fetch Drogon with minimal features using CPM
message(STATUS "Fetching Drogon from GitHub via CPM")

CPMAddPackage(
        NAME drogon
        GIT_REPOSITORY https://github.com/drogonframework/drogon.git
        GIT_TAG v1.9.8
        OPTIONS
        "BUILD_ORM OFF"
        "BUILD_CTL OFF"
        "BUILD_EXAMPLES OFF"
        "BUILD_TESTING OFF"
        "BUILD_DOC OFF"
        "BUILD_BROTLI OFF"
        "BUILD_YAML_CONFIG OFF"
        "TRANTOR_USE_TLS none"
        "BUILD_C-ARES OFF"
        "USE_SPDLOG ON"
)

# Collect source files
file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/*.cc
)

# Create the executable
add_executable(homepage ${SOURCES})

# Set include directories for the target - now using src as the base
target_include_directories(homepage PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Find spdlog package
find_package(spdlog REQUIRED)

# Link with Drogon and spdlog statically
target_link_libraries(homepage drogon spdlog::spdlog_header_only)
