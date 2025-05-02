module;

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stack>
#include <stdexcept>
#include <optional>

export module ll1parser;

import token;
import symbol;
import parserbase;
import terminalfactory;

export class LL1ParseTree; // forward declaration

export using LL1PTChild = std::variant<Terminal, Token, LL1ParseTree>;

class LL1ParseTree {
    private:
        const NonTerminal nonTerminal;
        std::vector<LL1PTChild> children;
        bool hasProduction = false;

        bool placeInner(const Production& production) {
            if (!hasProduction) {
                if (nonTerminal == production.first) {
                    for (const auto& symbol : production.second) {
                        if (std::holds_alternative<Terminal>(symbol)) {
                            children.push_back(std::get<Terminal>(symbol));
                        } else if (std::holds_alternative<NonTerminal>(symbol)) {
                            children.push_back(std::get<NonTerminal>(symbol));
                        } else {
                            throw std::runtime_error("Unknown symbol type in production");
                        }
                    }
                    hasProduction = true;
                    return true;
                } else {
                    throw std::runtime_error("First non-terminal do not match production: " + std::string{nonTerminal.getName()} + " and " + std::string{production.first.getName()});
                }
            }
            for (auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    // Do nothing
                } else if (std::holds_alternative<LL1ParseTree>(child)) {
                    auto& childParseTree = std::get<LL1ParseTree>(child);
                    bool result = childParseTree.placeInner(production);
                    if (result) {
                        return true;
                    }
                } else if (std::holds_alternative<Terminal>(child)) {
                    throw std::runtime_error("First empty child of parse tree is a terminal when trying to place production");
                }
            }
            return false;
        }

        bool placeInner(const Token& token) {
            if (!hasProduction) {
                throw std::runtime_error("First empty child of parse tree is a non-terminal when trying to place token");
            }
            for (auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    // Do nothing
                } else if (std::holds_alternative<LL1ParseTree>(child)) {
                        bool result = std::get<LL1ParseTree>(child).placeInner(token);
                        if (result) {
                            return true;
                        }
                } else if (std::holds_alternative<Terminal>(child)) {
                    const auto& childTerminal = std::get<Terminal>(child);
                    if (childTerminal.matchesToken(token)) {
                        child.emplace<Token>(token);
                        return true;
                    } else {
                        throw std::runtime_error("First terminal do not match token: " + std::string{childTerminal.getName()} + " and " + token.toStringPrint());
                    }
                }
            }
            return false;
        }

    public:
        LL1ParseTree(const NonTerminal& nonTerminal) : nonTerminal(nonTerminal) {}

        void place(const Production& production) {
            bool result = placeInner(production);
            if (!result) {
                throw std::runtime_error("Failed to place production in parse tree");
            }
        }

        void place(const Token& token) {
            bool result = placeInner(token);
            if (!result) {
                throw std::runtime_error("Failed to place token in parse tree");
            }
        }

        NonTerminal getNonTerminal() const {
            return nonTerminal;
        }

        ParseTree toParseTree() const {
            ParseTree parseTree(nonTerminal);
            for (const auto& child : children) {
                if (std::holds_alternative<Token>(child)) {
                    parseTree.addChild(std::get<Token>(child));
                } else if (std::holds_alternative<LL1ParseTree>(child)) {
                    if (hasProduction) {
                        parseTree.addChild(std::get<LL1ParseTree>(child).toParseTree());
                    } else {
                        throw std::runtime_error("Cannot add non-terminal to parse tree");
                    }
                } else if (std::holds_alternative<Terminal>(child)) {
                    // It is assumed that all tokens become terminals at this point
                    throw std::runtime_error("Cannot add terminal to parse tree");
                } else {
                    throw std::runtime_error("Unknown child type in parse tree");
                }
            }
            return parseTree;
        }
};

export using LL1ParsingTable = std::map<std::pair<NonTerminal, TerminalOrEOL>, std::vector<SymbolOrEOL>>;

using LL1SymbolStack = std::stack<SymbolOrEOL>;

export class LL1Parser : public ParserBase {
    private:
        const NonTerminal startSymbol;
        const LL1ParsingTable parsingTable;

    public:
        LL1Parser(const NonTerminal startSymbol, const LL1ParsingTable& parsingTable)
            : startSymbol(startSymbol), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            auto nextTokenIter = tokenIter;
            LL1SymbolStack symbolStack;
            bool assumeEndOfLine = false;

            LL1ParseTree parseTree(startSymbol);
            symbolStack.push(startSymbol);

            while (true) {
                if (symbolStack.empty()) {
                    return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter};
                }

                if (nextTokenIter == tokenEnd) {
                    return ParserRejectResult{"Unexpected end of input", nextTokenIter};
                }

                const auto currentSymbol = symbolStack.top();

                if (std::holds_alternative<char>(currentSymbol)) {
                    // The top of symbol stack is EOL

                    symbolStack.pop();
                    return ParserAcceptResult{parseTree.toParseTree().withoutStartSymbol(), nextTokenIter, nextTokenIter};
                }
                else if (std::holds_alternative<Terminal>(currentSymbol)) {
                    // The top of symbol stack is a terminal
                    // Check if it matches current token

                    const auto& stackTerminal = std::get<Terminal>(currentSymbol);
                    if (stackTerminal.matchesToken(*nextTokenIter)) {
                        parseTree.place(*nextTokenIter);
                        nextTokenIter++;
                        symbolStack.pop();
                    } else {
                        return ParserRejectResult{"LL1 Unexpected token: " + nextTokenIter->toStringPrint(), nextTokenIter};
                    }
                } else if (std::holds_alternative<NonTerminal>(currentSymbol)) {
                    // The top of symbol stack is a non-terminal
                    // Look up parsing table for production rule

                    const auto stackNonTerminal = std::get<NonTerminal>(currentSymbol);

                    const auto findProduction = [&]()->std::optional<std::vector<SymbolOrEOL>> {
                        if (!assumeEndOfLine && nextTokenIter != tokenEnd) {
                            const auto& currentTokenAsTerminal = TerminalFactory::fromToken(*nextTokenIter);
                            const auto productionIter = parsingTable.find(std::make_pair(stackNonTerminal, currentTokenAsTerminal));
                            if (productionIter != parsingTable.end()) {
                                const auto& production = productionIter->second;
                                return production;
                            }
                        }
                        assumeEndOfLine = true;
                        const auto productionIter = parsingTable.find(std::make_pair(stackNonTerminal, EOL));
                        if (productionIter != parsingTable.end()) {
                            const auto& production = productionIter->second;
                            return production;
                        }
                        return std::nullopt;
                    };

                    const auto production = findProduction();
                    if (!production.has_value()) {
                        return ParserRejectResult{"No production found for non-terminal: " + std::string{stackNonTerminal.getName()}, nextTokenIter};
                    }

                    // Push symbols to temp stack
                    LL1SymbolStack tempStack;
                    for (const auto& symbol : production.value()) {
                        if (std::holds_alternative<Terminal>(symbol)) {
                            const auto& terminalSymbol = std::get<Terminal>(symbol);
                            tempStack.push(terminalSymbol);
                        } else if (std::holds_alternative<NonTerminal>(symbol)) {
                            const auto& nonTerminalSymbol = std::get<NonTerminal>(symbol);
                            tempStack.push(nonTerminalSymbol);
                        } else if (std::holds_alternative<char>(symbol)) {
                            const auto& eolSymbol = std::get<char>(symbol);
                            tempStack.push(eolSymbol);
                        }
                        else {
                            throw std::runtime_error("Unknown symbol type in production");
                        }
                    }

                    // Pop non-terminal
                    symbolStack.pop();

                    // Push symbols to actual stack in reverse order
                    while (!tempStack.empty()) {
                        symbolStack.push(tempStack.top());
                        tempStack.pop();
                    }

                    // Push symbols to parse tree
                    std::vector<Symbol> product;
                    for (const auto& symbol : production.value()) {
                        if (std::holds_alternative<Terminal>(symbol)) {
                            const auto& terminalSymbol = std::get<Terminal>(symbol);
                            product.push_back(terminalSymbol);
                        } else if (std::holds_alternative<NonTerminal>(symbol)) {
                            const auto& nonTerminalSymbol = std::get<NonTerminal>(symbol);
                            product.push_back(nonTerminalSymbol);
                        }
                    }
                    parseTree.place(Production{stackNonTerminal, product});
                }
                else {
                    throw std::runtime_error("Unknown symbol type in parsing stack");
                }
            }

            return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter};
        }
};
