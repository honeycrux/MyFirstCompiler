module;

#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <variant>

export module parser;

import token;
import parserbase;
import rdparser;
import ll1parser;
import slr1parser;
import terminalfactory;

export using ParserError = std::string;

export class Parser {
    private:
        static std::string formatPosition(const std::vector<Token>::const_iterator iter, const std::vector<Token>::const_iterator begin) {
            return " (token position " + std::to_string(std::distance(begin, iter) + 1) + ")";
        }

    public:
        std::variant<bool, ParserError> parse(const std::vector<Token>& tokens) const {
            const auto id = TerminalFactory::getIdentifier();
            const auto intLiteral = TerminalFactory::getIntegerLiteral();
            const auto floatLiteral = TerminalFactory::getFloatLiteral();
            const auto strLiteral = TerminalFactory::getStringLiteral();
            const auto getKeyword = TerminalFactory::getKeyword;
            const auto getOperator = TerminalFactory::getOperator;
            const auto getPunctuator = TerminalFactory::getPunctuator;

            const auto EOL = std::nullopt;

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
                            { NonTerminal("Expr"), NonTerminal("ArgList'") },
                            { NonTerminal("ArgList'") }
                        }
                    },
                    {
                        NonTerminal("ArgList'"),
                        {
                            { getPunctuator(","), NonTerminal("Expr"), NonTerminal("ArgList'") },
                            {}
                        }
                    },
                    {
                        NonTerminal("Factor"),
                        {
                            { getPunctuator("("), NonTerminal("Expr"), getPunctuator(")") },
                            { NonTerminal("VarConst") }
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
                    },

                    {
                        NonTerminal("VarConst"),
                        {
                            { varConstParser.get() }
                        }
                    }
                }
            );

            auto result = parser.parse(tokens.begin(), tokens.end());

            if (std::holds_alternative<ParserRejectResult>(result)) {
                const auto rejectResult = std::get<ParserRejectResult>(result);
                return ParserError(rejectResult.message + formatPosition(rejectResult.where, tokens.begin()));
            }
            else if (std::holds_alternative<ParserAcceptResult>(result)) {
                const auto acceptResult = std::get<ParserAcceptResult>(result);
                if (acceptResult.next != tokens.end()) {
                    return ParserError("Unexpected token: " + acceptResult.next->toStringPrint() + formatPosition(acceptResult.next, tokens.begin()));
                }
                return true;
            }
            else {
                throw std::runtime_error("Unexpected result type");
            }
        }
};
