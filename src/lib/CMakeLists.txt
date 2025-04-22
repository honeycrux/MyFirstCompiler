add_library(lib)
target_sources(lib
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    compiler.cpp
    lexer.cpp
    token.cpp
    tokenacceptor.cpp
    tokenregistry.cpp
)
