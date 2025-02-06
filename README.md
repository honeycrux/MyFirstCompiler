This is my attempt to write a simple compiler written using modern C++. It compiles with the most basic features using C as a reference.

## Notes on Installation
This is a CMake project. C++20 modules are used, so it requires versions of tools as recent as 2023/2024. There is a [blog post](https://www.kitware.com/import-cmake-the-experiment-is-over/) talking about it. I made a quick query to perplexity.ai and here's what it suggests:
> What are the required cmake, ninja, and clang versions for c++20 modules?
- CMake: CMake 3.28 or newer is required.
- Ninja: Ninja 1.11.1 or newer is required.
- Clang: Clang 16 or newer is required.

I use VSCode's integration with CMake to build the project.
