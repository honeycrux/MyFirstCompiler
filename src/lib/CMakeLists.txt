add_library(lib)
target_sources(lib
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    lexicalparser.cpp
    token.cpp
    tokenacceptor.cpp
    tokenfactory.cpp
)
