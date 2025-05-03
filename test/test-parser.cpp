// For my reference:
// https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top
// REQUIRE: Stops at first failure
// CHECK: Execution continues in the same test case even if assertion fails

#include <catch2/catch_all.hpp>
#include <string>
#include <memory>

import token;
import lexer;
import ast;
import parser;

inline std::vector<Token> getLexerOutput(const Lexer& lexer, const std::string_view code) {
    auto result = lexer.acceptCode(code);
    REQUIRE(std::holds_alternative<std::vector<Token>>(result));
    return std::get<std::vector<Token>>(result);
}

inline std::unique_ptr<AstNode> getParserOutput(const Lexer& lexer, const Parser& parser, const std::string_view code) {
    auto result = parser.parse(getLexerOutput(lexer, code));
    REQUIRE(std::holds_alternative<std::unique_ptr<AstNode>>(result));
    std::get<std::unique_ptr<AstNode>>(result)->toQuadrupleString(); // check that quadruples can be generated
    return std::move(std::get<std::unique_ptr<AstNode>>(result));
}

inline ParserError getParserError(const Lexer& lexer, const Parser& parser, const std::string_view code) {
    auto result = parser.parse(getLexerOutput(lexer, code));
    REQUIRE(std::holds_alternative<ParserError>(result));
    return std::get<ParserError>(result);
}

std::string wrapWithMain(const std::string& code) {
    return "int main() { " + code + " }";
}

TEST_CASE("Parse global declarations") {
    Lexer lexer;
    Parser parser;

    SECTION("Parse a function declaration") {
        std::string code = "int foo() { a = 1; }";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function declaration with parameters") {
        std::string code = "float foo(int a, float b, str c[]) { a = 1; }";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function declaration with parameters with trailing comma") {
        std::string code = "int foo(int a,) { a = 1; }";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function declaration with multiple statements") {
        std::string code = "int foo(int a,) { a = 1; if (a) { a = 1; } }";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a variable declaration") {
        std::string code = "int a;";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a variable declaration with initializer") {
        std::string code = "str s = \"foo\";";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a variable declaration of an array") {
        std::string code = "int a[10];";
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse multiple variable declarations") {
        std::string code = "int a = 1, b, c = 2; float d;";
        const auto ast = getParserOutput(lexer, parser, code);
    }
}

TEST_CASE("Parse statements") {
    Lexer lexer;
    Parser parser;

    SECTION("Parse an expression statement") {
        std::string code = wrapWithMain("a + b;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse an if statement") {
        std::string code = wrapWithMain("if (a) { a = 1; }");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse an if-else statement") {
        std::string code = wrapWithMain("if (a) { a = 1; } else { a = 1; }");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a while statement") {
        std::string code = wrapWithMain("while (a) { a = 1; }");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a for statement") {
        std::string code = wrapWithMain("for (i=0; i<10; i=i+1) { a = 1; }");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a for statement with multiple initializers") {
        std::string code = wrapWithMain("for (i=0, j=0; i<10; i=i+1) { a = 1; }");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a return statement") {
        std::string code = wrapWithMain("return;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a return statement with expression") {
        std::string code = wrapWithMain("return 0;");
        const auto ast = getParserOutput(lexer, parser, code);
    }
}

TEST_CASE("Parse expressions") {
    Lexer lexer;
    Parser parser;

    SECTION("Parse an empty expression") {
        std::string code = wrapWithMain(";");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a literal expression") {
        std::string code = wrapWithMain("\"hello world\";");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a variable expression") {
        std::string code = wrapWithMain("a;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse an assignment expression") {
        std::string code = wrapWithMain("a = 1;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse multiple assignment expressions") {
        std::string code = wrapWithMain("(a = b = c = 1);");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a binary expression") {
        std::string code = wrapWithMain("a == b;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a unary expression") {
        std::string code = wrapWithMain("-a;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse multiple arithmetic expressions") {
        std::string code = wrapWithMain("a + (b - c) * 12 / e;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse multiple relational/equality expressions") {
        std::string code = wrapWithMain("a < b && (c > d || e != f) && g <= h || i >= j && k == l;");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse multiple unary expressions") {
        std::string code = wrapWithMain("++-+(--a);");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function call expression") {
        std::string code = wrapWithMain("foo();");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function call expression with arguments") {
        std::string code = wrapWithMain("foo(a, b);");
        const auto ast = getParserOutput(lexer, parser, code);
    }

    SECTION("Parse a function call expression with trailing comma") {
        std::string code = wrapWithMain("foo(a,);");
        const auto ast = getParserOutput(lexer, parser, code);
    }
}

TEST_CASE("Parse errors") {
    Lexer lexer;
    Parser parser;

    SECTION("Parse an invalid expression") {
        std::string code = wrapWithMain("a +;");
        auto error = getParserError(lexer, parser, code);
    }

    SECTION("Parse an invalid statement") {
        std::string code = wrapWithMain("if (a) { a = 1; } else { a = 1; } else { a = 1; }");
        auto error = getParserError(lexer, parser, code);
    }
}
