add_library(lexer)
target_sources(lexer
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES

    token.cpp
    tokenacceptor.cpp
    tokenfactory.cpp
    tokenregistry.cpp
    lexer.cpp
)
