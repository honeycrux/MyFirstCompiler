include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(test-lexer test-lexer.cpp)
add_executable(test-parser test-parser.cpp)
add_executable(test-types test-types.cpp)
target_link_libraries(test-lexer PRIVATE Catch2::Catch2WithMain lexer)
target_link_libraries(test-parser PRIVATE Catch2::Catch2WithMain lexer parser)
target_link_libraries(test-types PRIVATE Catch2::Catch2WithMain lexer parser)

add_test(test-lexer test-lexer)
add_test(test-parser test-parser)
add_test(test-types test-types)
