# References
# https://www.youtube.com/watch?v=UkDGVb0QXMs
# https://www.kitware.com/import-cmake-the-experiment-is-over/
# https://stackoverflow.com/questions/57300495/how-to-use-c20-modules-with-cmake

cmake_minimum_required(VERSION 3.28)
project(compiler_project LANGUAGES CXX)

# Turning off extensions avoids an issue with the clang 16 compiler
# clang 17 and greater can avoid this setting
set(CMAKE_CXX_EXTENSIONS OFF)
# Set the version of C++ for the project
set(CMAKE_CXX_STANDARD 20)
# Enable scanning for module dependencies
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_subdirectory(src/lib)
add_subdirectory(src/bin)
