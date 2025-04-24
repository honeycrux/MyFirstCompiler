add_library(lib)
target_sources(lib
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES

    # Compiler
    compiler.cpp

    # Lexer
    lexer.cpp
    token.cpp
    tokenacceptor.cpp
    tokenfactory.cpp
    tokenregistry.cpp

    # Parser
    parserbase.cpp
    rdparser.cpp
    ll1parser.cpp
    slr1parser.cpp
    terminalfactory.cpp
)
