add_library(parser)
target_sources(parser
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES

    parserbase.cpp
    rdparser.cpp
    ll1parser.cpp
    slr1parser.cpp
    terminalfactory.cpp
    parser.cpp
)
target_link_libraries(parser PRIVATE lexer)
