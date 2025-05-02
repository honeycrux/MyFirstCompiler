module;

#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <iostream>
#include <functional>

export module parser;

import token;
import symbol;
import ast;
import parserbase;
import rdparser;
import ll1parser;
import slr1parser;
import terminalfactory;

export using ParserError = std::string;

export class Parser {
    public:
        std::variant<bool, ParserError> parse(const std::vector<Token>& tokens) const {
            const auto id = TerminalFactory::getIdentifier();
            const auto intLiteral = TerminalFactory::getIntegerLiteral();
            const auto floatLiteral = TerminalFactory::getFloatLiteral();
            const auto strLiteral = TerminalFactory::getStringLiteral();
            const auto getKeyword = TerminalFactory::getKeyword;
            const auto getOperator = TerminalFactory::getOperator;
            const auto getPunctuator = TerminalFactory::getPunctuator;

            std::unique_ptr<LL1Parser> varConstParser = std::make_unique<LL1Parser>(NonTerminal("S"), LL1ParsingTable{
                // S
                {
                    { NonTerminal("S"), intLiteral },
                    { NonTerminal("VarConst"), EOL }
                },
                {
                    { NonTerminal("S"), floatLiteral },
                    { NonTerminal("VarConst"), EOL }
                },
                {
                    { NonTerminal("S"), strLiteral },
                    { NonTerminal("VarConst"), EOL }
                },
                {
                    { NonTerminal("S"), id },
                    { NonTerminal("VarConst"), EOL }
                },

                // VarConst
                {
                    { NonTerminal("VarConst"), intLiteral },
                    { NonTerminal("Constant") }
                },
                {
                    { NonTerminal("VarConst"), floatLiteral },
                    { NonTerminal("Constant") }
                },
                {
                    { NonTerminal("VarConst"), strLiteral },
                    { NonTerminal("Constant") }
                },
                {
                    { NonTerminal("VarConst"), id },
                    { NonTerminal("Var") }
                },

                // Constant
                {
                    { NonTerminal("Constant"), intLiteral },
                    { intLiteral }
                },
                {
                    { NonTerminal("Constant"), floatLiteral },
                    { floatLiteral }
                },
                {
                    { NonTerminal("Constant"), strLiteral },
                    { strLiteral }
                },

                // Var
                {
                    { NonTerminal("Var"), id },
                    { id, NonTerminal("Var'") }
                },

                // Var'
                {
                    { NonTerminal("Var'"), EOL },
                    {}
                },
                {
                    { NonTerminal("Var'"), getPunctuator("[") },
                    { getPunctuator("["), intLiteral, getPunctuator("]") }
                }
            });

            std::unique_ptr<SLR1Parser> paramListParser = std::make_unique<SLR1Parser>(
                State("S0"),
                ProductionMap{
                    { 1, { NonTerminal("ParamList"), { NonTerminal("Param"), getPunctuator(","), NonTerminal("ParamList") } } },
                    { 2, { NonTerminal("ParamList"), { NonTerminal("Param") } } },
                    { 3, { NonTerminal("ParamList"), {} } },
                    { 4, { NonTerminal("Param"), { NonTerminal("Type"), NonTerminal("ParamVar") } } },
                    { 5, { NonTerminal("ParamVar"), { id, getPunctuator("["), getPunctuator("]") } } },
                    { 6, { NonTerminal("ParamVar"), { id } } },
                    { 7, { NonTerminal("Type"), { getKeyword("int") } } },
                    { 8, { NonTerminal("Type"), { getKeyword("float") } } },
                    { 9, { NonTerminal("Type"), { getKeyword("str") } } }
                },
                SLR1ParsingTable{
                    { { State("S0"), getKeyword("int") }, State("S4") },
                    { { State("S0"), getKeyword("float") }, State("S5") },
                    { { State("S0"), getKeyword("str") }, State("S6") },
                    { { State("S0"), EOL }, 3 },
                    { { State("S0"), NonTerminal("ParamList") }, State("S1") },
                    { { State("S0"), NonTerminal("Param") }, State("S2") },
                    { { State("S0"), NonTerminal("Type") }, State("S3") },

                    { { State("S1"), EOL }, ACCEPT },

                    { { State("S2"), getPunctuator(",") }, State("S7") },
                    { { State("S2"), EOL }, 2 },

                    { { State("S3"), id }, State("S9") },
                    { { State("S3"), NonTerminal("ParamVar") }, State("S8") },

                    { { State("S4"), id }, 7 },

                    { { State("S5"), id }, 8 },

                    { { State("S6"), id }, 8 },

                    { { State("S7"), getKeyword("int") }, State("S4") },
                    { { State("S7"), getKeyword("float") }, State("S5") },
                    { { State("S7"), getKeyword("str") }, State("S6") },
                    { { State("S7"), EOL }, 3 },
                    { { State("S7"), NonTerminal("ParamList") }, State("S10") },
                    { { State("S7"), NonTerminal("Param") }, State("S2") },
                    { { State("S7"), NonTerminal("Type") }, State("S3") },

                    { { State("S8"), getPunctuator(",") }, 4 },
                    { { State("S8"), EOL }, 4 },

                    { { State("S9"), getPunctuator(",") }, 6 },
                    { { State("S9"), getPunctuator("]") }, State("S11") },
                    { { State("S9"), EOL }, 6 },

                    { { State("S10"), EOL }, 1 },

                    { { State("S11"), getPunctuator("]") }, State("S12") },

                    { { State("S12"), getPunctuator(",") }, 5 },
                    { { State("S12"), EOL }, 5 },
                }
            );

            const RecursiveDescentParser parser(
                NonTerminal("Start"),
                RdpProductMap{
                    {
                        NonTerminal("Start"),
                        {
                            { NonTerminal("DeclList") }
                        }
                    },
                    {
                        NonTerminal("DeclList"),
                        {
                            { NonTerminal("Decl"), NonTerminal("DeclList") },
                            { NonTerminal("Decl") }
                        }
                    },
                    {
                        NonTerminal("Decl"),
                        {
                            { NonTerminal("FuncDef") },
                            { NonTerminal("VarDecl") }
                        }
                    },

                    {
                        NonTerminal("FuncDef"),
                        {
                            { NonTerminal("Type"), id, getPunctuator("("), paramListParser.get(), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },

                    {
                        NonTerminal("VarDecl"),
                        {
                            { NonTerminal("Type"), NonTerminal("VarAssignableList"), getPunctuator(";") }
                        }
                    },
                    {
                        NonTerminal("VarAssignableList"),
                        {
                            { NonTerminal("VarAssignable"), getPunctuator(","), NonTerminal("VarAssignableList") },
                            { NonTerminal("VarAssignable") }
                        }
                    },
                    {
                        NonTerminal("VarAssignable"),
                        {
                            { NonTerminal("Var"), getOperator("="), NonTerminal("Expr") },
                            { NonTerminal("Var") }
                        }
                    },

                    {
                        NonTerminal("Var"),
                        {
                            { id, getPunctuator("["), intLiteral, getPunctuator("]") },
                            { id }
                        }
                    },
                    {
                        NonTerminal("Type"),
                        {
                            { getKeyword("int") },
                            { getKeyword("float") },
                            { getKeyword("str") }
                        }
                    },

                    {
                        NonTerminal("BlockStmt"),
                        {
                            { getPunctuator("{"), NonTerminal("StmtList"), getPunctuator("}") }
                        }
                    },
                    {
                        NonTerminal("StmtList"),
                        {
                            { NonTerminal("Stmt"), NonTerminal("StmtList") },
                            {}
                        }
                    },
                    {
                        NonTerminal("Stmt"),
                        {
                            { NonTerminal("VarDecl") },
                            { NonTerminal("IfStmt") },
                            { NonTerminal("WhileStmt") },
                            { NonTerminal("ForStmt") },
                            { NonTerminal("ReturnStmt") },
                            { NonTerminal("Expr"), getPunctuator(";") },
                            { getPunctuator(";") }
                        }
                    },

                    {
                        NonTerminal("IfStmt"),
                        {
                            { getKeyword("if"), getPunctuator("("), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt"), getKeyword("else"), NonTerminal("BlockStmt") },
                            { getKeyword("if"), getPunctuator("("), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },

                    {
                        NonTerminal("WhileStmt"),
                        {
                            { getKeyword("while"), getPunctuator("("), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },

                    {
                        NonTerminal("ForStmt"),
                        {
                            { getKeyword("for"), getPunctuator("("), NonTerminal("ForVarDecl"), getPunctuator(";"), NonTerminal("Expr"), getPunctuator(";"), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },
                    {
                        NonTerminal("ForVarDecl"),
                        {
                            { NonTerminal("VarAssignList") }
                        }
                    },
                    {
                        NonTerminal("VarAssignList"),
                        {
                            { NonTerminal("VarAssign"), getPunctuator(","), NonTerminal("VarAssignList") },
                            { NonTerminal("VarAssign") },
                            {}
                        }
                    },
                    {
                        NonTerminal("VarAssign"),
                        {
                            { NonTerminal("Var"), getOperator("="), NonTerminal("Expr") }
                        }
                    },

                    {
                        NonTerminal("ReturnStmt"),
                        {
                            { getKeyword("return"), NonTerminal("Expr"), getPunctuator(";") },
                            { getKeyword("return"), getPunctuator(";") }
                        }
                    },

                    {
                        NonTerminal("Expr"),
                        {
                            { NonTerminal("AssignExpr") }
                        }
                    },
                    {
                        NonTerminal("AssignExpr"),
                        {
                            { NonTerminal("Var"), getOperator("="), NonTerminal("Expr") },
                            { NonTerminal("OrExpr") }
                        }
                    },
                    {
                        NonTerminal("OrExpr"),
                        {
                            { NonTerminal("AndExpr"), NonTerminal("OrExpr'") }
                        }
                    },
                    {
                        NonTerminal("OrExpr'"),
                        {
                            { getOperator("||"), NonTerminal("AndExpr"), NonTerminal("OrExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("AndExpr"),
                        {
                            { NonTerminal("EqualityExpr"), NonTerminal("AndExpr'") }
                        }
                    },
                    {
                        NonTerminal("AndExpr'"),
                        {
                            { getOperator("&&"), NonTerminal("EqualityExpr"), NonTerminal("AndExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("EqualityExpr"),
                        {
                            { NonTerminal("RelationalExpr"), NonTerminal("EqualityExpr'") }
                        }
                    },
                    {
                        NonTerminal("EqualityExpr'"),
                        {
                            { NonTerminal("EqualityOp"), NonTerminal("RelationalExpr"), NonTerminal("EqualityExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("RelationalExpr"),
                        {
                            { NonTerminal("SumExpr"), NonTerminal("RelationalExpr'") }
                        }
                    },
                    {
                        NonTerminal("RelationalExpr'"),
                        {
                            { NonTerminal("RelationalOp"), NonTerminal("SumExpr"), NonTerminal("RelationalExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("SumExpr"),
                        {
                            { NonTerminal("MulExpr"), NonTerminal("SumExpr'") }
                        }
                    },
                    {
                        NonTerminal("SumExpr'"),
                        {
                            { NonTerminal("SumOp"), NonTerminal("MulExpr"), NonTerminal("SumExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("MulExpr"),
                        {
                            { NonTerminal("UnaryExpr"), NonTerminal("MulExpr'") }
                        }
                    },
                    {
                        NonTerminal("MulExpr'"),
                        {
                            { NonTerminal("MulOp"), NonTerminal("UnaryExpr"), NonTerminal("MulExpr'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("UnaryExpr"),
                        {
                            { NonTerminal("UnaryOp"), NonTerminal("UnaryExpr") },
                            { NonTerminal("FuncCall") },
                        }
                    },
                    {
                        NonTerminal("FuncCall"),
                        {
                            { id, getPunctuator("("), NonTerminal("ArgList"), getPunctuator(")") },
                            { NonTerminal("Factor") }
                        }
                    },
                    {
                        NonTerminal("ArgList"),
                        {
                            { NonTerminal("Expr"), getPunctuator(","), NonTerminal("ArgList") },
                            { NonTerminal("Expr") },
                            {}
                        }
                    },
                    {
                        NonTerminal("Factor"),
                        {
                            { getPunctuator("("), NonTerminal("Expr"), getPunctuator(")") },
                            { varConstParser.get() }
                        }
                    },
                    {
                        NonTerminal("EqualityOp"),
                        {
                            { getOperator("==") },
                            { getOperator("!=") }
                        }
                    },
                    {
                        NonTerminal("RelationalOp"),
                        {
                            { getOperator("<") },
                            { getOperator("<=") },
                            { getOperator(">") },
                            { getOperator(">=") }
                        }
                    },
                    {
                        NonTerminal("SumOp"),
                        {
                            { getOperator("+") },
                            { getOperator("-") }
                        }
                    },
                    {
                        NonTerminal("MulOp"),
                        {
                            { getOperator("*") },
                            { getOperator("/") }
                        }
                    },
                    {
                        NonTerminal("UnaryOp"),
                        {
                            { getOperator("+") },
                            { getOperator("-") },
                            { getOperator("!") }
                        }
                    }
                }
            );

            const SimplifyInstructionMap simplifyInstructionMap{
                { NonTerminal("Start"), SimplifyInstruction::RETAIN },
                { NonTerminal("DeclList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("Decl"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("FuncDef"), SimplifyInstruction::RETAIN },
                { NonTerminal("ParamList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("Param"), SimplifyInstruction::RETAIN },
                { NonTerminal("ParamVar"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("VarDecl"), SimplifyInstruction::RETAIN },
                { NonTerminal("VarAssignableList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("VarAssignable"), SimplifyInstruction::RETAIN },
                { NonTerminal("Var"), SimplifyInstruction::RETAIN },
                { NonTerminal("Type"), SimplifyInstruction::RETAIN },
                { NonTerminal("BlockStmt"), SimplifyInstruction::RETAIN },
                { NonTerminal("StmtList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("Stmt"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("IfStmt"), SimplifyInstruction::RETAIN },
                { NonTerminal("WhileStmt"), SimplifyInstruction::RETAIN },
                { NonTerminal("ForStmt"), SimplifyInstruction::RETAIN },
                { NonTerminal("ForVarDecl"), SimplifyInstruction::RETAIN },
                { NonTerminal("VarAssignList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("VarAssign"), SimplifyInstruction::RETAIN },
                { NonTerminal("ReturnStmt"), SimplifyInstruction::RETAIN },
                { NonTerminal("Expr"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("AssignExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("OrExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("OrExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("AndExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("AndExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("EqualityExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("EqualityExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("RelationalExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("RelationalExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("SumExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("SumExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("MulExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("MulExpr'"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("UnaryExpr"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("FuncCall"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("ArgList"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("Factor"), SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN },
                { NonTerminal("EqualityOp"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("RelationalOp"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("SumOp"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("MulOp"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("UnaryOp"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("VarConst"), SimplifyInstruction::MERGE_UP },
                { NonTerminal("Constant"), SimplifyInstruction::RETAIN },
                { NonTerminal("Var'"), SimplifyInstruction::MERGE_UP }
            };

            const AstHandlerMap handlerMap{
                {
                    NonTerminal("Start"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> astChildren;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                astChildren.push_back(parseTree.toAst());
                            }
                        }
                        std::unique_ptr<AstNode> start = std::make_unique<Start>(std::move(astChildren));
                        return start;
                    }
                },
                {
                    NonTerminal("FuncDef"), [](const SPTChildren& children) {
                        const auto& type = std::get<SimpleParseTree>(children[0]);
                        const auto& id = std::get<Token>(children[1]);
                        std::vector<std::unique_ptr<AstNode>> params;
                        for (int i = 3; i < children.size() - 2; i += 2) {
                            const auto& param = std::get<SimpleParseTree>(children[i]);
                            params.push_back(param.toAst());
                        }
                        const auto& blockStmt = std::get<SimpleParseTree>(children[children.size() - 1]);
                        std::unique_ptr<AstNode> funcDef = std::make_unique<FuncDef>(type.toAst(), id, std::move(params), blockStmt.toAst());
                        return funcDef;
                    }
                },
                {
                    NonTerminal("Param"), [](const SPTChildren& children) {
                        const auto& type = std::get<SimpleParseTree>(children[0]);
                        const auto& id = std::get<Token>(children[1]);
                        bool array = children.size() > 2;
                        std::unique_ptr<AstNode> param = std::make_unique<Param>(type.toAst(), id, array);
                        return param;
                    }
                },
                {
                    NonTerminal("VarDecl"), [](const SPTChildren& children) {
                        const auto& type = std::get<SimpleParseTree>(children[0]);
                        std::vector<std::unique_ptr<AstNode>> varAssignables;
                        for (int i = 1; i < children.size() - 1; i += 2) {
                            const auto& varAssignable = std::get<SimpleParseTree>(children[i]);
                            varAssignables.push_back(varAssignable.toAst());
                        }
                        std::unique_ptr<AstNode> varDecl = std::make_unique<VarDecl>(type.toAst(), std::move(varAssignables));
                        return varDecl;
                    }
                },
                {
                    NonTerminal("VarAssignable"), [](const SPTChildren& children) {
                        const auto& var = std::get<SimpleParseTree>(children[0]);
                        std::optional<std::unique_ptr<AstNode>> expr;
                        if (children.size() > 1) {
                            const auto& exprChild = std::get<SimpleParseTree>(children[2]);
                            expr = exprChild.toAst();
                        }
                        std::unique_ptr<AstNode> varAssignable = std::make_unique<VarAssignable>(var.toAst(), std::move(expr));
                        return varAssignable;
                    }
                },
                {
                    NonTerminal("Var"), [](const SPTChildren& children) {
                        const auto& id = std::get<Token>(children[0]);
                        std::optional<Token> arrayIndex;
                        if (children.size() > 1) {
                            arrayIndex.emplace(std::get<Token>(children[2]));
                        }
                        std::unique_ptr<AstNode> var = std::make_unique<Var>(id, arrayIndex);
                        return var;
                    }
                },
                {
                    NonTerminal("Type"), [](const SPTChildren& children) {
                        const auto& type = std::get<Token>(children[0]);
                        std::unique_ptr<AstNode> typeNode = std::make_unique<Type>(type);
                        return typeNode;
                    }
                },
                {
                    NonTerminal("Constant"), [](const SPTChildren& children) {
                        const auto& value = std::get<Token>(children[0]);
                        std::unique_ptr<AstNode> constant = std::make_unique<Constant>(value);
                        return constant;
                    }
                },
                {
                    NonTerminal("BlockStmt"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> statements;
                        for (int i = 1; i < children.size() - 1; ++i) {
                            const auto& child = children[i];
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                statements.push_back(parseTree.toAst());
                            }
                        }
                        std::unique_ptr<AstNode> blockStmt = std::make_unique<BlockStmt>(std::move(statements));
                        return blockStmt;
                    }
                },
                {
                    NonTerminal("IfStmt"), [](const SPTChildren& children) {
                        const auto& expr = std::get<SimpleParseTree>(children[2]);
                        const auto& blockStmt = std::get<SimpleParseTree>(children[4]);
                        std::optional<std::unique_ptr<AstNode>> elseBlockStmt;
                        if (children.size() > 6) {
                            const auto& elseBlock = std::get<SimpleParseTree>(children[6]);
                            elseBlockStmt = elseBlock.toAst();
                        }
                        std::unique_ptr<AstNode> ifStmt = std::make_unique<IfStmt>(expr.toAst(), blockStmt.toAst(), std::move(elseBlockStmt));
                        return ifStmt;
                    }
                },
                {
                    NonTerminal("WhileStmt"), [](const SPTChildren& children) {
                        const auto& expr = std::get<SimpleParseTree>(children[2]);
                        const auto& blockStmt = std::get<SimpleParseTree>(children[4]);
                        std::unique_ptr<AstNode> whileStmt = std::make_unique<WhileStmt>(expr.toAst(), blockStmt.toAst());
                        return whileStmt;
                    }
                },
                {
                    NonTerminal("ForStmt"), [](const SPTChildren& children) {
                        const auto& forVarDecl = std::get<SimpleParseTree>(children[2]);
                        const auto& condExpr = std::get<SimpleParseTree>(children[4]);
                        const auto& incrExpr = std::get<SimpleParseTree>(children[6]);
                        const auto& blockStmt = std::get<SimpleParseTree>(children[8]);
                        std::unique_ptr<AstNode> forStmt = std::make_unique<ForStmt>(forVarDecl.toAst(), condExpr.toAst(), incrExpr.toAst(), blockStmt.toAst());
                        return forStmt;
                    }
                },
                {
                    NonTerminal("ForVarDecl"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> varAssigns;
                        for (int i = 0; i < children.size(); i += 2) {
                            const auto& varAssign = std::get<SimpleParseTree>(children[i]);
                            varAssigns.push_back(varAssign.toAst());
                        }
                        std::unique_ptr<AstNode> forVarDecl = std::make_unique<ForVarDecl>(std::move(varAssigns));
                        return forVarDecl;
                    }
                },
                {
                    NonTerminal("VarAssign"), [](const SPTChildren& children) {
                        const auto& var = std::get<SimpleParseTree>(children[0]);
                        const auto& expr = std::get<SimpleParseTree>(children[2]);
                        std::unique_ptr<AstNode> varAssign = std::make_unique<VarAssign>(var.toAst(), expr.toAst());
                        return varAssign;
                    }
                },
                {
                    NonTerminal("ReturnStmt"), [](const SPTChildren& children) {
                        std::optional<std::unique_ptr<AstNode>> expr;
                        if (children.size() > 2) {
                            const auto& exprChild = std::get<SimpleParseTree>(children[1]);
                            expr = exprChild.toAst();
                        }
                        std::unique_ptr<AstNode> returnStmt = std::make_unique<ReturnStmt>(std::move(expr));
                        return returnStmt;
                    }
                },
                {
                    NonTerminal("AssignExpr"), [](const SPTChildren& children) {
                        const auto& var = std::get<SimpleParseTree>(children[0]);
                        const auto& expr = std::get<SimpleParseTree>(children[2]);
                        std::unique_ptr<AstNode> assignExpr = std::make_unique<AssignExpr>(var.toAst(), expr.toAst());
                        return assignExpr;
                    }
                },
                {
                    NonTerminal("OrExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> orExprs;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                orExprs.push_back(parseTree.toAst());
                            }
                        }
                        std::unique_ptr<AstNode> orExpr = std::make_unique<OrExpr>(std::move(orExprs[0]), std::move(orExprs[1]));
                        for (int i = 2; i < orExprs.size(); ++i) {
                            orExpr = std::make_unique<OrExpr>(std::move(orExpr), std::move(orExprs[i]));
                        }
                        return orExpr;
                    }
                },
                {
                    NonTerminal("AndExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> andExprs;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                andExprs.push_back(parseTree.toAst());
                            }
                        }
                        std::unique_ptr<AstNode> andExpr = std::make_unique<AndExpr>(std::move(andExprs[0]), std::move(andExprs[1]));
                        for (int i = 2; i < andExprs.size(); ++i) {
                            andExpr = std::make_unique<AndExpr>(std::move(andExpr), std::move(andExprs[i]));
                        }
                        return andExpr;
                    }
                },
                {
                    NonTerminal("EqualityExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> equalityExprs;
                        std::vector<std::string> equalityOps;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                equalityExprs.push_back(parseTree.toAst());
                            }
                            if (std::holds_alternative<Token>(child)) {
                                const auto& token = std::get<Token>(child);
                                equalityOps.push_back(token.getValue());
                            }
                        }
                        std::unique_ptr<AstNode> equalityExpr = equalityOps[0] == "==" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<EqualExpr>(std::move(equalityExprs[0]), std::move(equalityExprs[1]))) :
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<NotEqualExpr>(std::move(equalityExprs[0]), std::move(equalityExprs[1])));
                        for (int i = 2; i < equalityExprs.size(); ++i) {
                            equalityExpr = equalityOps[i - 1] == "==" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<EqualExpr>(std::move(equalityExpr), std::move(equalityExprs[i]))) :
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<NotEqualExpr>(std::move(equalityExpr), std::move(equalityExprs[i])));
                        }
                        return equalityExpr;
                    }
                },
                {
                    NonTerminal("RelationalExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> relationalExprs;
                        std::vector<std::string> relationalOps;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                relationalExprs.push_back(parseTree.toAst());
                            }
                            if (std::holds_alternative<Token>(child)) {
                                const auto& token = std::get<Token>(child);
                                relationalOps.push_back(token.getValue());
                            }
                        }
                        std::unique_ptr<AstNode> relationalExpr = relationalOps[0] == "<" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<LessExpr>(std::move(relationalExprs[0]), std::move(relationalExprs[1]))) :
                            relationalOps[0] == "<=" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<LessEqualExpr>(std::move(relationalExprs[0]), std::move(relationalExprs[1]))) :
                            relationalOps[0] == ">" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<GreaterExpr>(std::move(relationalExprs[0]), std::move(relationalExprs[1]))) :
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<GreaterEqualExpr>(std::move(relationalExprs[0]), std::move(relationalExprs[1])));
                        for (int i = 2; i < relationalExprs.size(); ++i) {
                            relationalExpr = relationalOps[i - 1] == "<" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<LessExpr>(std::move(relationalExpr), std::move(relationalExprs[i]))) :
                                relationalOps[i - 1] == "<=" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<LessEqualExpr>(std::move(relationalExpr), std::move(relationalExprs[i]))) :
                                relationalOps[i - 1] == ">" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<GreaterExpr>(std::move(relationalExpr), std::move(relationalExprs[i]))) :
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<GreaterEqualExpr>(std::move(relationalExpr), std::move(relationalExprs[i])));
                        }
                        return relationalExpr;
                    }
                },
                {
                    NonTerminal("SumExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> sumExprs;
                        std::vector<std::string> sumOps;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                sumExprs.push_back(parseTree.toAst());
                            }
                            if (std::holds_alternative<Token>(child)) {
                                const auto& token = std::get<Token>(child);
                                sumOps.push_back(token.getValue());
                            }
                        }
                        std::unique_ptr<AstNode> sumExpr = sumOps[0] == "+" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<AddExpr>(std::move(sumExprs[0]), std::move(sumExprs[1]))) :
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<SubExpr>(std::move(sumExprs[0]), std::move(sumExprs[1])));
                        for (int i = 2; i < sumExprs.size(); ++i) {
                            sumExpr = sumOps[i - 1] == "+" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<AddExpr>(std::move(sumExpr), std::move(sumExprs[i]))) :
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<SubExpr>(std::move(sumExpr), std::move(sumExprs[i])));
                        }
                        return sumExpr;
                    }
                },
                {
                    NonTerminal("MulExpr"), [](const SPTChildren& children) {
                        std::vector<std::unique_ptr<AstNode>> mulExprs;
                        std::vector<std::string> mulOps;
                        for (const auto& child : children) {
                            if (std::holds_alternative<SimpleParseTree>(child)) {
                                const auto& parseTree = std::get<SimpleParseTree>(child);
                                mulExprs.push_back(parseTree.toAst());
                            }
                            if (std::holds_alternative<Token>(child)) {
                                const auto& token = std::get<Token>(child);
                                mulOps.push_back(token.getValue());
                            }
                        }
                        std::unique_ptr<AstNode> mulExpr = mulOps[0] == "*" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<MulExpr>(std::move(mulExprs[0]), std::move(mulExprs[1]))) :
                            mulOps[0] == "/" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<DivExpr>(std::move(mulExprs[0]), std::move(mulExprs[1]))) :
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<ModExpr>(std::move(mulExprs[0]), std::move(mulExprs[1])));
                        for (int i = 2; i < mulExprs.size(); ++i) {
                            mulExpr = mulOps[i - 1] == "*" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<MulExpr>(std::move(mulExpr), std::move(mulExprs[i]))) :
                                mulOps[i - 1] == "/" ?
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<DivExpr>(std::move(mulExpr), std::move(mulExprs[i]))) :
                                static_cast<std::unique_ptr<AstNode>>(std::make_unique<ModExpr>(std::move(mulExpr), std::move(mulExprs[i])));
                        }
                        return mulExpr;
                    }
                },
                {
                    NonTerminal("UnaryExpr"), [](const SPTChildren& children) {
                        const auto& unaryOp = std::get<Token>(children[0]);
                        const auto& expr = std::get<SimpleParseTree>(children[1]);
                        std::string op = unaryOp.getValue();
                        std::unique_ptr<AstNode> unaryExpr = op == "+" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<UnaryPlusExpr>(expr.toAst())) :
                            op == "-" ?
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<UnaryMinusExpr>(expr.toAst())) :
                            static_cast<std::unique_ptr<AstNode>>(std::make_unique<NotExpr>(expr.toAst()));
                        return unaryExpr;
                    }
                },
                {
                    NonTerminal("FuncCall"), [](const SPTChildren& children) {
                        const auto& id = std::get<Token>(children[0]);
                        std::vector<std::unique_ptr<AstNode>> exprs;
                        for (int i = 2; i < children.size() - 1; i += 2) {
                            const auto& arg = std::get<SimpleParseTree>(children[i]);
                            exprs.push_back(arg.toAst());
                        }
                        std::unique_ptr<AstNode> funcCall = std::make_unique<FuncCall>(id, std::move(exprs));
                        return funcCall;
                    }
                },
                {
                    NonTerminal("Factor"), [](const SPTChildren& children) {
                        const auto& expr = std::get<SimpleParseTree>(children[1]);
                        return expr.toAst();
                    }
                }
            };

            auto result = parser.parse(tokens.begin(), tokens.end());

            if (std::holds_alternative<ParserRejectResult>(result)) {
                const auto rejectResult = std::get<ParserRejectResult>(result);
                return ParserError(rejectResult.message + " (at position " + rejectResult.where->getPosition() + ")");
            }
            else if (std::holds_alternative<ParserAcceptResult>(result)) {
                const auto acceptResult = std::get<ParserAcceptResult>(result);
                if (acceptResult.next != tokens.end()) {
                    return ParserError("Error: parsing ended before the end of program (" + acceptResult.next->getPosition() + ")");
                }
                std::cout << acceptResult.parseTree.toString() << std::endl;
                const auto simplified = acceptResult.parseTree.simplify(simplifyInstructionMap, handlerMap);
                std::cout << simplified.toString() << std::endl;
                const auto ast = simplified.toAst();
                std::cout << ast->toQuadrupleString() << std::endl;
                // const auto ast = acceptResult.parseTree.toAst(handlerMap);
                const auto typeCheckResult = ast->startTypeCheck();
                if (std::holds_alternative<TypeCheckError>(typeCheckResult)) {
                    const auto typeCheckError = std::get<TypeCheckError>(typeCheckResult);
                    return ParserError(typeCheckError.message + " (" + typeCheckError.where + ")");
                }
                return true;
            }
            else {
                throw std::runtime_error("Unexpected result type");
            }
        }
};
