module;

#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <algorithm>
#include <map>
#include <memory>
#include <functional>
#include <iostream>

export module parserbase;

import symbol;
import ast;
import token;

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

export class SimpleParseTree; // forward declaration
export class ParseTree; // forward declaration

export enum SimplifyInstruction {
    RETAIN,
    MERGE_UP,
    RETAIN_IF_MULTIPLE_CHILDREN,
};
export using SimplifyInstructionMap = std::map<NonTerminal, SimplifyInstruction>;

export using SPTChildren = std::vector<std::variant<Token, SimpleParseTree>>;

export using AstHandler = std::function<std::unique_ptr<AstNode>(const SPTChildren& children)>;
export using AstHandlerMap = std::map<NonTerminal, AstHandler>;

class SimpleParseTree {
    private:
        const NonTerminal nonTerminal;
        std::vector<std::variant<Token, SimpleParseTree>> children;
        const AstHandlerMap astHandlerMap;

    public:
        SimpleParseTree(const NonTerminal& nonTerminal, const AstHandlerMap& astHandlerMap) : nonTerminal(nonTerminal), astHandlerMap(astHandlerMap) {}

        NonTerminal getNonTerminal() const {
            return nonTerminal;
        }

        void addChild(const std::variant<Token, SimpleParseTree>& child) {
            children.push_back(child);
        }

        std::unique_ptr<AstNode> toAst() const {
            auto handlerIter = astHandlerMap.find(nonTerminal);
            if (handlerIter == astHandlerMap.end()) {
                throw std::runtime_error("No handler found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            const auto& handler = handlerIter->second;

            return handler(children);
        }

        std::string toString() const {
            std::ostringstream oss;
            const auto visitor = overloads{
                [&oss](const Token& token) { oss << token.toStringPrint(); },
                [&oss](const SimpleParseTree& parseTree) { oss << parseTree.toString(); }
            };
            oss << nonTerminal.getName() << "( ";
            for (auto child = children.begin(); child != children.end(); ++child) {
                if (child != children.begin()) {
                    oss << ", ";
                }
                std::visit(visitor, *child);
            }
            oss << " )";
            return oss.str();
        }
};

class ParseTree {
    private:
        const NonTerminal nonTerminal;
        std::vector<std::variant<Token, ParseTree>> children;

        std::vector<std::variant<Token, SimpleParseTree>> simplifyInner(const SimplifyInstructionMap& instructionMap, const AstHandlerMap& astHandlerMap) const {
            // Simplify the children first
            std::vector<std::variant<Token, SimpleParseTree>> simplified;
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    simplified.push_back(std::get<Token>(child));
                } else if (std::holds_alternative<ParseTree>(child)) {
                    auto parseTreeChild = std::get<ParseTree>(child);
                    auto simplifiedChildren = parseTreeChild.simplifyInner(instructionMap, astHandlerMap);
                    for (const auto& simplifiedChild : simplifiedChildren) {
                        simplified.push_back(simplifiedChild);
                    }
                }
            }

            // Make simplification based on instruction
            auto instructionIter = instructionMap.find(nonTerminal);
            if (instructionIter == instructionMap.end()) {
                throw std::runtime_error("No instruction found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            const auto& instruction = instructionIter->second;
            bool toMergeUp = instruction == SimplifyInstruction::MERGE_UP;
            if (instruction == SimplifyInstruction::RETAIN_IF_MULTIPLE_CHILDREN) {
                toMergeUp = simplified.size() < 2;
            }
            if (toMergeUp) {
                return simplified;
            } else {
                SimpleParseTree simplifiedTree(nonTerminal, astHandlerMap);
                for (const auto& child : simplified) {
                    simplifiedTree.addChild(child);
                }
                return { simplifiedTree };
            }
        }

    public:
        ParseTree(const NonTerminal& nonTerminal) : nonTerminal(nonTerminal) {}

        NonTerminal getNonTerminal() const {
            return nonTerminal;
        }

        void addChild(const std::variant<Token, ParseTree>& child) {
            children.push_back(child);
        }

        ParseTree withoutStartSymbol() const {
            if (children.size() == 1 && std::holds_alternative<ParseTree>(children[0])) {
                return std::get<ParseTree>(children[0]);
            }
            throw std::runtime_error("Parse tree of " + std::string{nonTerminal.getName()} + " have "
                + std::to_string(children.size()) + " children, expected 1");
        }

        SimpleParseTree simplify(const SimplifyInstructionMap& instructionMap, const AstHandlerMap& astHandlerMap) const {
            const auto simplified = simplifyInner(instructionMap, astHandlerMap);
            if (simplified.size() == 1 && std::holds_alternative<SimpleParseTree>(simplified[0])) {
                return std::get<SimpleParseTree>(simplified[0]);
            }
            throw std::runtime_error("Error when simplifying parse tree");
        }

        std::string toString() const {
            std::ostringstream oss;
            const auto visitor = overloads{
                [&oss](const Token& token) { oss << token.toStringPrint(); },
                [&oss](const ParseTree& parseTree) { oss << parseTree.toString(); }
            };
            oss << nonTerminal.getName() << "( ";
            for (auto child = children.begin(); child != children.end(); ++child) {
                if (child != children.begin()) {
                    oss << ", ";
                }
                std::visit(visitor, *child);
            }
            oss << " )";
            return oss.str();
        }
};

export struct ParserAcceptResult {
    ParseTree parseTree;
    std::vector<Token>::const_iterator next;
    std::vector<Token>::const_iterator bestIter;
};

export struct ParserRejectResult {
    std::string message;
    std::vector<Token>::const_iterator where;
};

export using ParsingResult = std::variant<ParserAcceptResult, ParserRejectResult>;

export class ParserBase {
    public:
        ParserBase() {}
        virtual ~ParserBase() = default;
        virtual ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const = 0;
};
