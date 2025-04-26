add_executable(compiler)
target_sources(compiler
  PUBLIC
    FILE_SET all_my_modules TYPE CXX_MODULES FILES

    compiler.cpp
)
target_link_libraries(compiler PRIVATE lexer parser)
