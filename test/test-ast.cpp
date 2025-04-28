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
        CHECK(typeCheckResult.type == INT_T);
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
        CHECK(typeCheckResult.type == FLOAT_T);
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
        CHECK(typeCheckResult.type == NONE_T);
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
        std::cout << std::get<TypeCheckError>(result).message << std::endl;
    }
}
