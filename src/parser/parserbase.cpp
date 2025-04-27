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

export class ParseTree; // forward declaration

// export using PTChildren = std::vector<std::variant<Token, ParseTree>>;
// export using SimplifyHandler = std::function<PTChildren(const PTChildren& simplifiedChildren)>;
export enum SimplifyInstruction {
    RETAIN,
    MERGE_UP,
    RETAIN_IF_MULTIPLE_CHILDREN,
};
export using SimplifyInstructionMap = std::map<NonTerminal, SimplifyInstruction>;

class ParseTree {
    private:
        const NonTerminal nonTerminal;
        std::vector<std::variant<Token, ParseTree>> children;

        std::vector<std::variant<Token, ParseTree>> simplifyInner(const SimplifyInstructionMap& instructionMap) const {
            // Simplify the children first
            std::vector<std::variant<Token, ParseTree>> simplified;
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    simplified.push_back(std::get<Token>(child));
                } else if (std::holds_alternative<ParseTree>(child)) {
                    auto parseTreeChild = std::get<ParseTree>(child);
                    auto simplifiedChildren = parseTreeChild.simplifyInner(instructionMap);
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
                ParseTree simplifiedTree(nonTerminal);
                for (const auto& child : simplified) {
                    simplifiedTree.addChild(child);
                }
                return { simplifiedTree };
            }
        }

        std::unique_ptr<AstNode> toAst(std::map<NonTerminal, AstHandler> handlerMap) const {
            auto handlerIter = handlerMap.find(nonTerminal);
            if (handlerIter == handlerMap.end()) {
                throw std::runtime_error("No handler found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            const auto& handler = handlerIter->second;

            AstChildren astChildren{};
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    astChildren.push_back(std::get<Token>(child));
                } else if (std::holds_alternative<ParseTree>(child)) {
                    auto astChild = std::get<ParseTree>(child).toAst(handlerMap);
                    astChildren.push_back(std::move(astChild));
                }
            }

            return handler(astChildren);
        }

    public:
        ParseTree(const NonTerminal& nonTerminal) : nonTerminal(nonTerminal) {}

        NonTerminal getNonTerminal() const {
            return nonTerminal;
        }

        void addChild(const std::variant<Token, ParseTree>& child) {
            children.push_back(child);
        }

        ParseTree simplify(const SimplifyInstructionMap& instructionMap) const {
            const auto simplified = simplifyInner(instructionMap);
            if (simplified.size() == 1 && std::holds_alternative<ParseTree>(simplified[0])) {
                return std::get<ParseTree>(simplified[0]);
            }
            throw std::runtime_error("Error when simplifying parse tree");
        }

        ParseTree withoutStartSymbol() const {
            if (children.size() == 1 && std::holds_alternative<ParseTree>(children[0])) {
                return std::get<ParseTree>(children[0]);
            }
            throw std::runtime_error("Parse tree of " + std::string{nonTerminal.getName()} + " have "
                + std::to_string(children.size()) + " children, expected 1");
        }

        std::string toString() const {
            std::ostringstream oss;
            const auto visitor = overloads{
                [&oss](const Token& token) {
                    oss << token.toStringPrint();
                },
                [&oss](const ParseTree& parseTree) {
                    oss << parseTree.toString();
                }
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

export class ImperfectParseTree; // forward declaration

export using IPTChild = std::variant<Terminal, Token, ImperfectParseTree>;

class ImperfectParseTree {
    private:
        const NonTerminal nonTerminal;
        std::vector<IPTChild> children;

    public:
        ImperfectParseTree(const NonTerminal& nonTerminal) : nonTerminal(nonTerminal) {}

        NonTerminal getNonTerminal() const {
            return nonTerminal;
        }

        IPTChild* addChild(const IPTChild& child) {
            children.push_back(child);
            return children.data() + children.size() - 1;
        }

        ParseTree toParseTree() const {
            ParseTree parseTree(nonTerminal);
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    parseTree.addChild(std::get<Token>(child));
                } else if (std::holds_alternative<ImperfectParseTree>(child)) {
                    parseTree.addChild(std::get<ImperfectParseTree>(child).toParseTree());
                } else if (std::holds_alternative<Terminal>(child)) {
                    // It is assumed that all tokens become terminals at this point
                    throw std::runtime_error("Cannot add terminal to parse tree");
                } else {
                    throw std::runtime_error("Unknown child type in parse tree");
                }
            }
            return parseTree;
        }

        void printSummary() const {
            std::cout << "ImperfectParseTree: " << nonTerminal.getName() << std::endl;
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    std::cout << "  Token: " << std::get<Token>(child).toStringPrint() << std::endl;
                } else if (std::holds_alternative<ImperfectParseTree>(child)) {
                    std::get<ImperfectParseTree>(child).printSummary();
                } else if (std::holds_alternative<Terminal>(child)) {
                    std::cout << "  Terminal: " << std::get<Terminal>(child).getName() << std::endl;
                } else {
                    throw std::runtime_error("Unknown child type in imperfect parse tree");
                }
            }
        }
};

export struct ParserAcceptResult {
    ParseTree parseTree;
    std::vector<Token>::const_iterator next;
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
