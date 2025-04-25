module;

#include <memory>
#include <vector>
#include <map>

export module parser;

import token;
import parserbase;
import rdparser;
import ll1parser;
import slr1parser;
import terminalfactory;

export class Parser {
    public:
        ParsingResult parse(const std::vector<Token>& tokens) {
            const auto id = TerminalFactory::getIdentifier();
            const auto intLiteral = TerminalFactory::getIntegerLiteral();
            const auto floatLiteral = TerminalFactory::getFloatLiteral();
            const auto stringLiteral = TerminalFactory::getStringLiteral();
            const auto getKeyword = TerminalFactory::getKeyword;
            const auto getOperator = TerminalFactory::getOperator;
            const auto getPunctuator = TerminalFactory::getPunctuator;

            std::unique_ptr<LL1Parser> exprParser = std::make_unique<LL1Parser>(NonTerminal("Expr"), LL1ParsingTable{});

            std::unique_ptr<SLR1Parser> paramListParser = std::make_unique<SLR1Parser>(State("S0"), SLR1ParsingTable{});

            const RecursiveDescentParser parser(
                NonTerminal("Start"),
                {
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
                            { NonTerminal("Type"), id, getPunctuator("("), NonTerminal("ParamList"), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },
                    {
                        NonTerminal("ParamList"),
                        {
                            { paramListParser.get() }
                        }
                    },
                    {
                        NonTerminal("VarDecl"),
                        {
                            { NonTerminal("Type"), NonTerminal("VarList"), getPunctuator(";") }
                        }
                    },
                    {
                        NonTerminal("VarList"),
                        {
                            { NonTerminal("VarAssignable"), getPunctuator(","), NonTerminal("VarList") },
                            { NonTerminal("VarAssignable") }
                        }
                    },
                    {
                        NonTerminal("VarAssignable"),
                        {
                            { NonTerminal("Var"), getPunctuator("="), NonTerminal("Expr") },
                            { NonTerminal("Var") }
                        }
                    },
                    {
                        NonTerminal("Var"),
                        {
                            { id, getPunctuator("["), NonTerminal("Expr"), getPunctuator("]") },
                            { id }
                        }
                    },
                    {
                        NonTerminal("Type"),
                        {
                            { getKeyword("int") },
                            { getKeyword("float") },
                            { getKeyword("string") }
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
                            { getPunctuator(";") },
                            { NonTerminal("BlockStmt") }
                        }
                    },
                    {
                        NonTerminal("IfStmt"),
                        {
                            { getKeyword("if"), getPunctuator("("), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt") },
                            { getKeyword("if"), getPunctuator("("), NonTerminal("Expr"), getPunctuator(")"), NonTerminal("BlockStmt"), getKeyword("else"), NonTerminal("BlockStmt") }
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
                            { getKeyword("for"), getPunctuator("("), NonTerminal("ForVarDecl"), getPunctuator(";"), NonTerminal("Expr"), getPunctuator(";"), NonTerminal("ExprList"), getPunctuator(")"), NonTerminal("BlockStmt") }
                        }
                    },
                    {
                        NonTerminal("ForVarDecl"),
                        {
                            { NonTerminal("Type"), NonTerminal("VarList") },
                            { NonTerminal("VarList") }
                        }
                    },
                    {
                        NonTerminal("ExprList"),
                        {
                            { NonTerminal("Expr"), getPunctuator(","), NonTerminal("ExprList") },
                            { NonTerminal("Expr") },
                            {}
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
                            { exprParser.get() }
                        }
                    }
                }
            );

            auto result = parser.parse(tokens.begin(), tokens.end());

            return result;
        }
};
