// For my reference:
// https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top
// REQUIRE: Stops at first failure
// CHECK: Execution continues in the same test case even if assertion fails

#include <catch2/catch_all.hpp>
#include <string>

import token;
import lexer;
import ast;
import parser;

using Catch::Matchers::ContainsSubstring;

inline std::vector<Token> getLexerOutput(const Lexer& lexer, const std::string_view code) {
    auto result = lexer.acceptCode(code);
    REQUIRE(std::holds_alternative<std::vector<Token>>(result));
    return std::get<std::vector<Token>>(result);
}

inline std::unique_ptr<AstNode> getParserOutput(const Lexer& lexer, const Parser& parser, const std::string_view code) {
    auto result = parser.parse(getLexerOutput(lexer, code));
    REQUIRE(std::holds_alternative<std::unique_ptr<AstNode>>(result));
    return std::move(std::get<std::unique_ptr<AstNode>>(result));
}

inline TypeCheckSuccess getTypeOutput(const Lexer& lexer, const Parser& parser, const std::string_view code) {
    auto ast = getParserOutput(lexer, parser, code);
    auto result = ast->startTypeCheck();
    REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
    return std::get<TypeCheckSuccess>(result);
}

inline TypeCheckError getTypeError(const Lexer& lexer, const Parser& parser, const std::string_view code) {
    auto ast = getParserOutput(lexer, parser, code);
    auto result = ast->startTypeCheck();
    REQUIRE(std::holds_alternative<TypeCheckError>(result));
    return std::get<TypeCheckError>(result);
}

std::string wrapWithMain(const std::string& code) {
    return "int main() { " + code + " }";
}

TEST_CASE("Access global definitions") {
    Lexer lexer;
    Parser parser;

    SECTION("Access a global variable") {
        std::string code = "int a = 1; int main() { a; }";
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Access a global function") {
        std::string code = "int foo() { return 1; } int main() { foo(); }";
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Access an undeclared global function") {
        std::string code = "int main() { foo(); }";
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Function not found"));
    }
}

TEST_CASE("Access local definitions") {
    Lexer lexer;
    Parser parser;

    SECTION("Access a local variable") {
        std::string code = "int main() { int a, b; a; b; }";
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Access an undeclared variable") {
        std::string code = "int main() { a; }";
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));
    }

    SECTION("Access another function's local variable") {
        std::string code = "int foo() { int a; } int main() { a; }";
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));
    }
}

TEST_CASE("Variable declaration and assignment statements") {
    Lexer lexer;
    Parser parser;

    SECTION("Assignment of the correct type") {
        std::string code = wrapWithMain("int a = 1;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Assignment of the wrong type") {
        std::string code = wrapWithMain("int a = \"foo\";");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Reassignment of the correct type") {
        std::string code = wrapWithMain("int a; a = 1;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Reassignment of the wrong type") {
        std::string code = wrapWithMain("int a; a = \"foo\";");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Redefinition of a global variable") {
        std::string code = wrapWithMain("int a; a = 1; float a; a = 2.2;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Redefinition of a global function") {
        std::string code = "int foo() { return 1; } str foo() { return \"foo\"; }";
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Redefinition of a local variable") {
        std::string code = "int main() { int a; a = 1; float a; a = 2.2; }";
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = "int main() { int a; a = 1; float a; a = 2; }";
        const auto error = getTypeError(lexer, parser, code2);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }
}

TEST_CASE("Array indexing") {
    Lexer lexer;
    Parser parser;

    SECTION("Assignment of the correct type with array index") {
        std::string code = wrapWithMain("int a[10]; a[0] = 1;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Assignment of the wrong type with array index") {
        std::string code = wrapWithMain("int a[10]; a[0] = \"foo\";");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));

        std::string code2 = wrapWithMain("int a = 1.0;");
        const auto error2 = getTypeError(lexer, parser, code2);
        CHECK_THAT(error2.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Assignment of non-array type to an array") {
        std::string code = wrapWithMain("int a[10]; a = 1;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Array variable used without index"));
    }

    SECTION("Array index with a non-array type") {
        std::string code = wrapWithMain("int a; a[0] = 1;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Non-array variable used with index"));
    }

    SECTION("Array index with variable") {
        std::string code = wrapWithMain("int a[10]; int b[20]; a[b[0]] = 1;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Array index with non-integer variable") {
        std::string code = wrapWithMain("int a[10]; float b; a[b] = 1;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Array index must be int"));
    }
}

TEST_CASE("Expression statements") {
    Lexer lexer;
    Parser parser;

    SECTION("Logical expression with correct operand types") {
        std::string code = wrapWithMain("int a; int b; a && b;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Logical expression with correct operand types") {
        std::string code = wrapWithMain("int a; int b; int c; (a < b) || !(a > c);");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Relational expression comparing correct types") {
        std::string code = wrapWithMain("int a; float b; a < b;");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("int a; float b; a > b;");
        const auto type2 = getTypeOutput(lexer, parser, code2);

        std::string code3 = wrapWithMain("str a; str b; a <= b;");
        const auto type3 = getTypeOutput(lexer, parser, code);
    }

    SECTION("Relational expression comparing different types") {
        std::string code = wrapWithMain("int a; str b; a >= b;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Equality expression comparing the correct type") {
        std::string code = wrapWithMain("int a; float b; a == b;");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("str a; str b; a != b;");
        const auto type2 = getTypeOutput(lexer, parser, code);
    }

    SECTION("Equality expression comparing different types") {
        std::string code = wrapWithMain("int a; str b; a != b;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Expression statement with incorrect type") {
        std::string code = wrapWithMain("int a; a + \"foo\";");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Cannot add types int and str"));
    }

    SECTION("Expression that evaluates to an integer") {
        std::string code = wrapWithMain("int a; int b; int c = a + b * b;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Expression that evaluates to a float") {
        std::string code = wrapWithMain("int a; float b; float c = a - b / a % b;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Expression that evaluates to a string") {
        std::string code = wrapWithMain("str a; str b; str c = a + b;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Unary expression with correct type") {
        std::string code = wrapWithMain("int a; int b; +a;");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("int a; int b; a + (-b);");
        const auto type2 = getTypeOutput(lexer, parser, code);
    }

    SECTION("Unary expression with incorrect type") {
        std::string code = wrapWithMain("str a; -a;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("The operand must be numeric"));
    }

    SECTION("Assignment expression with correct type") {
        std::string code = wrapWithMain("int a; int b; a = b;");
        const auto type = getTypeOutput(lexer, parser, code);
    }

    SECTION("Assignment expression with incorrect type") {
        std::string code = wrapWithMain("int a; float b; a = a + b;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Type mismatch"));
    }

    SECTION("Expression with undeclared variable") {
        std::string code = wrapWithMain("int a = a;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));
    }

    SECTION("Function call on a non-function") {
        std::string code = wrapWithMain("int a; a();");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Function call on a non-function"));
    }

    SECTION("Using the result of a function call") {
        std::string code = "str foo() { return \"foo\"; } int main() { str a = foo() + \"bar\"; }";
        const auto type = getTypeOutput(lexer, parser, code);
    }
}

TEST_CASE("If statements") {
    Lexer lexer;
    Parser parser;

    SECTION("If statement with correct condition") {
        std::string code = wrapWithMain("if (1 < 2) { 1; }");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("int a; if (a + a) { a = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code);
    }

    SECTION("If statement with incorrect condition") {
        std::string code = wrapWithMain("float a; if (a) { }");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Condition must be boolean"));
    }

    SECTION("If creates scope") {
        std::string code = wrapWithMain("int a; if (1 < 2) { int b; } b;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));

        std::string code2 = wrapWithMain("int a; if (1 < 2) { int b; b = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code2);
    }
}

TEST_CASE("While statements") {
    Lexer lexer;
    Parser parser;

    SECTION("While statement with correct condition") {
        std::string code = wrapWithMain("while (1 < 2) { 1; }");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("int a; while (a + a) { a = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code);
    }

    SECTION("While statement with incorrect condition") {
        std::string code = wrapWithMain("float a; while (a) { }");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Condition must be boolean"));
    }

    SECTION("While creates scope") {
        std::string code = wrapWithMain("int a; while (1 < 2) { int b; } b;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));

        std::string code2 = wrapWithMain("int a; while (1 < 2) { int b; b = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code2);
    }
}

TEST_CASE("For statements") {
    Lexer lexer;
    Parser parser;

    SECTION("For statement with correct condition and initializers") {
        std::string code = wrapWithMain("int i; for (i = 10; i; i = i - 1) { }");
        const auto type = getTypeOutput(lexer, parser, code);

        std::string code2 = wrapWithMain("int a, b; for (a = 0, b = 0; a < 10 && b >= 0; b = a = a + 1) { a = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code);
    }

    SECTION("For statement with incorrect condition") {
        std::string code = wrapWithMain("float a; for (a = 0.0; a; a = a + 1) { }");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Condition must be boolean"));
    }

    SECTION("For statement with incorrect initialization") {
        std::string code = wrapWithMain("int a; for (a = 0, b = 0; a < 10; a = a + 1) { }");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));
    }

    SECTION("For creates scope") {
        std::string code = wrapWithMain("int b; for (b = 0; b < 10; b = b + 1) { int c; } c;");
        const auto error = getTypeError(lexer, parser, code);
        CHECK_THAT(error.message, ContainsSubstring("Variable not found"));

        std::string code2 = wrapWithMain("int b; for (b = 0; b < 10; b + 1) { int c; c = 1; }");
        const auto type2 = getTypeOutput(lexer, parser, code2);
    }
}
