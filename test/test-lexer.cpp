// For my reference:
// https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top
// REQUIRE: Stops at first failure
// CHECK: Execution continues in the same test case even if assertion fails

#include <catch2/catch_all.hpp>
#include <string>

import token;
import lexer;

using Catch::Matchers::ContainsSubstring;

inline std::string getLexerOutput(const Lexer& lexer, const std::string_view code) {
    auto result = lexer.acceptCode(code);
    REQUIRE(std::holds_alternative<std::vector<Token>>(result));
    auto tokens = std::get<std::vector<Token>>(result);
    return lexer.getPrintString(tokens);
}

inline LexerError getLexerError(const Lexer& lexer, const std::string_view code) {
    auto result = lexer.acceptCode(code);
    REQUIRE(std::holds_alternative<LexerError>(result));
    auto lexerError = std::get<LexerError>(result);
    return lexerError;
}

TEST_CASE("Parse an identifier") {
    Lexer lexer;

    SECTION("Parse a correct identifier") {
        std::string code = "_hello12k";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<_hello12k, identifier>");
    }

    SECTION("Parse an incorrect identifier") {
        std::string code = "1abc";
        auto error = getLexerError(lexer, code);
    }

    SECTION("Parse an identifier that looks like a keyword") {
        std::string code = "if_";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<if_, identifier>");
    }
}

TEST_CASE("Parse a numeric literal") {
    Lexer lexer;

    SECTION("Parse an integer") {
        std::string code = "12";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<12, integer>");
    }

    SECTION("Parse a float") {
        std::string code = "12.04";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<12.04, float>");
    }
}

TEST_CASE("Parse a string literal") {
    Lexer lexer;

    SECTION("Parse a correct string") {
        std::string code = "\"hello\"";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<\"hello\", string>");
    }

    SECTION("Parse a unterminated string") {
        std::string code = "\"hello";
        auto error = getLexerError(lexer, code);
    }

    SECTION("Parse escaped characters") {
        std::string code = "\"\\n,\\t,\\\",\\\\,\"";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<\"\\n,\\t,\\\",\\\\,\", string>");
    }

    SECTION("Parse unterminated escape") {
        std::string code = "\"\\\"";
        auto error = getLexerError(lexer, code);
    }
}

TEST_CASE("Parse a keyword") {
    Lexer lexer;

    SECTION("Parse keyword") {
        auto printout = getLexerOutput(lexer, "do");
        CHECK(printout == "<do, keyword>");
    }

    SECTION("Parse longest keyword") {
        auto printout = getLexerOutput(lexer, "float");
        CHECK(printout == "<float, keyword>");
    }
}

TEST_CASE("Parse an operator") {
    Lexer lexer;

    SECTION("Parse operator") {
        auto printout = getLexerOutput(lexer, ">");
        CHECK(printout == "<>, operator>");
    }

    SECTION("Parse longest operator") {
        auto printout = getLexerOutput(lexer, ">=");
        CHECK(printout == "<>=, operator>");
    }

    SECTION("Parse consecutive operators") {
        auto printout = getLexerOutput(lexer, "**");
        CHECK(printout == "<*, operator>, <*, operator>");
    }
}

TEST_CASE("Parse a punctuator") {
    Lexer lexer;

    SECTION("Parse consecutive punctuators") {
        auto printout = getLexerOutput(lexer, "{}");
        CHECK(printout == "<{, punctuator>, <}, punctuator>");
    }
}

TEST_CASE("Lexer returns error position") {
    Lexer lexer;
    std::string code = "12.04.04";
    auto error = getLexerError(lexer, code);
    CHECK_THAT(error, ContainsSubstring("(at position 1:6)"));
}

TEST_CASE("Parse a code snippet") {
    Lexer lexer;

    SECTION("Parse a statement") {
        std::string code = "str c = 1;";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<str, keyword>, <c, identifier>, <=, operator>, <1, integer>, <;, punctuator>");
    }

    SECTION("Parse a code snippet") {
        std::string code = "\nint main()  \n{\n\treturn 0;  \n  }\n\n";
        auto printout = getLexerOutput(lexer, code);
        CHECK(printout == "<int, keyword>, <main, identifier>, <(, punctuator>, <), punctuator>, <{, punctuator>, <return, keyword>, <0, integer>, <;, punctuator>, <}, punctuator>");
    }

    SECTION("Parse a code snippet with error") {
        std::string code = "int main() { return 0.0.0; }";
        auto error = getLexerError(lexer, code);
    }
}
