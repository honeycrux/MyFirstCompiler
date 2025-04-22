// For my reference:
// https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top
// REQUIRE: Stops at first failure
// CHECK: Execution continues in the same test case even if assertion fails

#include <catch2/catch_all.hpp>
#include <string>

import token;
import lexer;

using Catch::Matchers::ContainsSubstring;

struct ParserOutput {
    std::string printOutput;
    std::string writeOutput;
};

inline ParserOutput getParserOutput(const Lexer& parser, const std::string_view code) {
    auto result = parser.acceptCode(code);
    REQUIRE(std::holds_alternative<std::vector<Token>>(result));
    auto tokens = std::get<std::vector<Token>>(result);
    return { parser.getPrintString(tokens), parser.getWriteString(tokens) };
}

inline LexicalError getParserError(const Lexer& parser, const std::string_view code) {
    auto result = parser.acceptCode(code);
    REQUIRE(std::holds_alternative<LexicalError>(result));
    auto lexicalError = std::get<LexicalError>(result);
    return lexicalError;
}

TEST_CASE("Parse an identifier") {
    Lexer parser;

    SECTION("Parse a correct identifier") {
        std::string code = "_hello12k";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<_hello12k, identifier>");
        CHECK(writeout == "<_hello12k, 0>");
    }

    SECTION("Parse an incorrect identifier") {
        std::string code = "1abc";
        auto error = getParserError(parser, code);
    }

    SECTION("Parse an identifier that looks like a keyword") {
        std::string code = "if_";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<if_, identifier>");
        CHECK(writeout == "<if_, 0>");
    }
}

TEST_CASE("Parse a numeric literal") {
    Lexer parser;

    SECTION("Parse an integer") {
        std::string code = "12";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<12, number>");
        CHECK(writeout == "<12, 1>");
    }

    SECTION("Parse a float") {
        std::string code = "12.04";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<12.04, number>");
        CHECK(writeout == "<12.04, 1>");
    }
}

TEST_CASE("Parse a string literal") {
    Lexer parser;

    SECTION("Parse a correct string") {
        std::string code = "\"hello\"";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<\"hello\", string>");
        CHECK(writeout == "<\"hello\", 2>");
    }

    SECTION("Parse a unterminated string") {
        std::string code = "\"hello";
        auto error = getParserError(parser, code);
    }

    SECTION("Parse escaped characters") {
        std::string code = "\"\\n,\\t,\\\",\\\\,\"";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<\"\\n,\\t,\\\",\\\\,\", string>");
        CHECK(writeout == "<\"\\n,\\t,\\\",\\\\,\", 2>");
    }

    SECTION("Parse unterminated escape") {
        std::string code = "\"\\\"";
        auto error = getParserError(parser, code);
    }
}

TEST_CASE("Parse a keyword") {
    Lexer parser;

    SECTION("Parse keyword") {
        auto [printout, writeout] = getParserOutput(parser, "do");
        CHECK(printout == "<do, keyword>");
        CHECK(writeout == "<do, 108>");
    }

    SECTION("Parse longest keyword") {
        auto [printout, writeout] = getParserOutput(parser, "double");
        CHECK(printout == "<double, keyword>");
        CHECK(writeout == "<double, 101>");
    }
}

TEST_CASE("Parse an operator") {
    Lexer parser;

    SECTION("Parse operator") {
        auto [printout, writeout] = getParserOutput(parser, ">");
        CHECK(printout == "<>, operator>");
        CHECK(writeout == "<>, 205>");
    }

    SECTION("Parse longest operator") {
        auto [printout, writeout] = getParserOutput(parser, ">=");
        CHECK(printout == "<>=, operator>");
        CHECK(writeout == "<>=, 203>");
    }

    SECTION("Parse consecutive operators") {
        auto [printout, writeout] = getParserOutput(parser, "**");
        CHECK(printout == "<*, operator>, <*, operator>");
        CHECK(writeout == "<*, 209>, <*, 209>");
    }
}

TEST_CASE("Parse a punctuator") {
    Lexer parser;

    SECTION("Parse consecutive punctuators") {
        auto [printout, writeout] = getParserOutput(parser, "{}");
        CHECK(printout == "<{, punctuator>, <}, punctuator>");
        CHECK(writeout == "<{, 300>, <}, 301>");
    }
}

TEST_CASE("Lexical parser returns error position") {
    Lexer parser;
    std::string code = "12.04.04";
    auto error = getParserError(parser, code);
    CHECK_THAT(error, ContainsSubstring("(at position 6)"));
}

TEST_CASE("Parse a code snippet") {
    Lexer parser;

    SECTION("Parse a statement") {
        std::string code = "string c = 1;";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<string, keyword>, <c, identifier>, <=, operator>, <1, number>, <;, punctuator>");
        CHECK(writeout == "<string, 102>, <c, 0>, <=, 200>, <1, 1>, <;, 303>");
    }

    SECTION("Parse a code snippet") {
        std::string code = "\nint main()  \n{\n\treturn 0;  \n  }\n\n";
        auto [printout, writeout] = getParserOutput(parser, code);
        CHECK(printout == "<int, keyword>, <main, identifier>, <(, punctuator>, <), punctuator>, <{, punctuator>, <return, keyword>, <0, number>, <;, punctuator>, <}, punctuator>");
        CHECK(writeout == "<int, 100>, <main, 0>, <(, 304>, <), 305>, <{, 300>, <return, 106>, <0, 1>, <;, 303>, <}, 301>");
    }

    SECTION("Parse a code snippet with error") {
        std::string code = "int main() { return 0.0.0; }";
        auto error = getParserError(parser, code);
    }
}
