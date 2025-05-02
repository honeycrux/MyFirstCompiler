// For my reference:
// https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top
// REQUIRE: Stops at first failure
// CHECK: Execution continues in the same test case even if assertion fails

#include <catch2/catch_all.hpp>
#include <string>
#include <memory>
#include <iostream>

import tokenfactory;
import ast;

TEST_CASE("Type check arithmetic expression") {
    SECTION("Type check with integer literals") {
        std::string fakeCode = "1 + 2";
        std::string_view fakeCodeView = fakeCode;
        const auto one = TokenFactory::getIntegerLiteralToken("1", fakeCodeView.begin(), fakeCodeView.begin());
        const auto two = TokenFactory::getIntegerLiteralToken("2", fakeCodeView.begin(), fakeCodeView.begin());
        std::unique_ptr<AstNode> addExpr = std::make_unique<AddExpr>(
            std::make_unique<Constant>(one),
            std::make_unique<Constant>(two)
        );
        auto result = addExpr->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
        auto typeCheckResult = std::get<TypeCheckSuccess>(result);
        CHECK(typeCheckResult.type == DataType::INT_T);
    }

    SECTION("Type check with complex expression") {
        std::string fakeCode = "1 + 2.5 * 6";
        std::string_view fakeCodeView = fakeCode;
        const auto one = TokenFactory::getIntegerLiteralToken("1", fakeCodeView.begin(), fakeCodeView.begin());
        const auto two = TokenFactory::getFloatLiteralToken("2.5", fakeCodeView.begin(), fakeCodeView.begin());
        const auto six = TokenFactory::getIntegerLiteralToken("6", fakeCodeView.begin(), fakeCodeView.begin());
        std::unique_ptr<AstNode> addExpr = std::make_unique<AddExpr>(
            std::make_unique<Constant>(one),
            std::make_unique<MulExpr>(
                std::make_unique<Constant>(two),
                std::make_unique<Constant>(six)
            )
        );
        auto result = addExpr->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
        auto typeCheckResult = std::get<TypeCheckSuccess>(result);
        CHECK(typeCheckResult.type == DataType::FLOAT_T);
    }
}

TEST_CASE("Type check assignments") {
    SECTION("Assign a correct type") {
        std::string fakeCode = "int x; x = 1;";
        std::string_view fakeCodeView = fakeCode;
        const auto intType = TokenFactory::findKeywordToken("int", fakeCodeView.begin(), fakeCodeView.begin());
        const auto x = TokenFactory::getIdentifierToken("x", fakeCodeView.begin(), fakeCodeView.begin());
        const auto one = TokenFactory::getIntegerLiteralToken("1", fakeCodeView.begin(), fakeCodeView.begin());

        std::unique_ptr<AstNode> varAssignable = std::make_unique<VarAssignable>(std::make_unique<Var>(x, std::nullopt), std::nullopt);
        std::vector<std::unique_ptr<AstNode>> vec;
        vec.push_back(std::move(varAssignable));
        std::unique_ptr<AstNode> varDecl = std::make_unique<VarDecl>(
            std::make_unique<Type>(*intType),
            std::move(vec)
        );

        std::unique_ptr<AstNode> var2 = std::make_unique<Var>(x, std::nullopt);
        std::unique_ptr<AstNode> constantOne = std::make_unique<Constant>(one);
        std::unique_ptr<AstNode> assignExpr = std::make_unique<AssignExpr>(std::move(var2), std::move(constantOne));
        std::vector<std::unique_ptr<AstNode>> vec2;
        vec2.push_back(std::move(varDecl));
        vec2.push_back(std::move(assignExpr));
        std::unique_ptr<AstNode> blockStmt = std::make_unique<BlockStmt>(std::move(vec2));

        auto result = blockStmt->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
        auto typeCheckResult = std::get<TypeCheckSuccess>(result);
        CHECK(typeCheckResult.type == DataType::NONE_T);
    }

    SECTION("Assign an incorrect type") {
        std::string fakeCode = "int x; x = 1.5;";
        std::string_view fakeCodeView = fakeCode;
        const auto intType = TokenFactory::findKeywordToken("int", fakeCodeView.begin(), fakeCodeView.begin());
        const auto x = TokenFactory::getIdentifierToken("x", fakeCodeView.begin(), fakeCodeView.begin());
        const auto one = TokenFactory::getFloatLiteralToken("1.5", fakeCodeView.begin(), fakeCodeView.begin());

        std::unique_ptr<AstNode> varAssignable = std::make_unique<VarAssignable>(std::make_unique<Var>(x, std::nullopt), std::nullopt);
        std::vector<std::unique_ptr<AstNode>> vec;
        vec.push_back(std::move(varAssignable));
        std::unique_ptr<AstNode> varDecl = std::make_unique<VarDecl>(
            std::make_unique<Type>(*intType),
            std::move(vec)
        );

        std::unique_ptr<AstNode> var2 = std::make_unique<Var>(x, std::nullopt);
        std::unique_ptr<AstNode> constantOne = std::make_unique<Constant>(one);
        std::unique_ptr<AstNode> assignExpr = std::make_unique<AssignExpr>(std::move(var2), std::move(constantOne));
        std::vector<std::unique_ptr<AstNode>> vec2;
        vec2.push_back(std::move(varDecl));
        vec2.push_back(std::move(assignExpr));
        std::unique_ptr<AstNode> blockStmt = std::make_unique<BlockStmt>(std::move(vec2));

        auto result = blockStmt->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckError>(result));
    }
}

TEST_CASE("Type check function call") {
    SECTION("Function call with correct types") {
        std::string fakeCode = "int foo(int a, float b) { }";
        std::string_view fakeCodeView = fakeCode;
        const auto intType = TokenFactory::findKeywordToken("int", fakeCodeView.begin(), fakeCodeView.begin());
        const auto floatType = TokenFactory::findKeywordToken("float", fakeCodeView.begin(), fakeCodeView.begin());
        const auto foo = TokenFactory::getIdentifierToken("foo", fakeCodeView.begin(), fakeCodeView.begin());
        const auto a = TokenFactory::getIdentifierToken("a", fakeCodeView.begin(), fakeCodeView.begin());
        const auto b = TokenFactory::getIdentifierToken("b", fakeCodeView.begin(), fakeCodeView.begin());

        std::unique_ptr<AstNode> paramA = std::make_unique<Param>(std::make_unique<Type>(*intType), a, false);
        std::unique_ptr<AstNode> paramB = std::make_unique<Param>(std::make_unique<Type>(*floatType), b, false);
        std::vector<std::unique_ptr<AstNode>> params;
        params.push_back(std::move(paramA));
        params.push_back(std::move(paramB));

        std::vector<std::unique_ptr<AstNode>> funcBody;

        std::unique_ptr<AstNode> funcDecl = std::make_unique<FuncDef>(
            std::make_unique<Type>(*intType),
            foo,
            std::move(params),
            std::make_unique<BlockStmt>(std::move(funcBody))
        );

        auto result = funcDecl->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
    }

    SECTION("Function can be called") {
        std::string fakeCode = "int foo() { } int main() { foo(); }";
        std::string_view fakeCodeView = fakeCode;
        const auto intType = TokenFactory::findKeywordToken("int", fakeCodeView.begin(), fakeCodeView.begin());
        const auto foo = TokenFactory::getIdentifierToken("foo", fakeCodeView.begin(), fakeCodeView.begin());
        const auto main = TokenFactory::getIdentifierToken("main", fakeCodeView.begin(), fakeCodeView.begin());

        std::vector<std::unique_ptr<AstNode>> params;
        std::vector<std::unique_ptr<AstNode>> funcBody;

        std::unique_ptr<AstNode> funcDecl = std::make_unique<FuncDef>(
            std::make_unique<Type>(*intType),
            foo,
            std::move(params),
            std::make_unique<BlockStmt>(std::move(funcBody))
        );

        std::vector<std::unique_ptr<AstNode>> params2;
        std::vector<std::unique_ptr<AstNode>> funcBody2;
        std::vector<std::unique_ptr<AstNode>> args;
        std::unique_ptr<AstNode> funcCall = std::make_unique<FuncCall>(foo, std::move(args));
        funcBody2.push_back(std::move(funcCall));

        std::unique_ptr<AstNode> funcDecl2 = std::make_unique<FuncDef>(
            std::make_unique<Type>(*intType),
            main,
            std::move(params2),
            std::make_unique<BlockStmt>(std::move(funcBody2))
        );

        std::vector<std::unique_ptr<AstNode>> funcDecls;
        funcDecls.push_back(std::move(funcDecl));
        funcDecls.push_back(std::move(funcDecl2));
        std::unique_ptr<AstNode> start = std::make_unique<Start>(std::move(funcDecls));

        auto result = start->startTypeCheck();
        REQUIRE(std::holds_alternative<TypeCheckSuccess>(result));
    }
}
