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

export using LL1ParsingTable = std::map<std::pair<NonTerminal, TerminalOrEOL>, std::vector<SymbolOrEOL>>;

using LL1SymbolStack = std::stack<std::pair<SymbolOrEOL, IPTChild*>>;

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

            IPTChild parseTree{ImperfectParseTree(startSymbol)};
            symbolStack.push(std::make_pair(startSymbol, &parseTree));

            while (true) {
                if (symbolStack.empty()) {
                    return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter->formatPosition()};
                }

                if (nextTokenIter == tokenEnd) {
                    return ParserRejectResult{"Unexpected end of input", nextTokenIter->formatPosition()};
                }

                const auto& currentStackTop = symbolStack.top();
                const auto& currentSymbol = currentStackTop.first;
                const auto& currentPtr = currentStackTop.second;

                if (std::holds_alternative<char>(currentSymbol)) {
                    // The top of symbol stack is EOL

                    
                    symbolStack.pop();
                    return ParserAcceptResult{std::get<ImperfectParseTree>(parseTree).toParseTree().withoutStartSymbol(), nextTokenIter};
                }
                else if (std::holds_alternative<Terminal>(currentSymbol)) {
                    // The top of symbol stack is a terminal
                    // Check if it matches current token

                    const auto& stackTerminal = std::get<Terminal>(currentSymbol);
                    if (stackTerminal.matchesToken(*nextTokenIter)) {
                        if (currentPtr == nullptr) {
                            throw std::runtime_error("Invalid terminal pointer for terminal: " + std::string{stackTerminal.getName()});
                        }

                        currentPtr->emplace<Token>(*nextTokenIter);
                        nextTokenIter++;
                        const auto& foo = std::get<Token>(*currentPtr);
                        symbolStack.pop();
                    } else {
                        return ParserRejectResult{"Unexpected token: " + nextTokenIter->toStringPrint(), nextTokenIter->formatPosition()};
                    }
                } else if (std::holds_alternative<NonTerminal>(currentSymbol)) {
                    // The top of symbol stack is a non-terminal
                    // Look up parsing table for production rule

                    const auto& stackNonTerminal = std::get<NonTerminal>(currentSymbol);
                    if (currentPtr == nullptr || !std::holds_alternative<ImperfectParseTree>(*currentPtr)) {
                        throw std::runtime_error("Invalid parse tree pointer for non-terminal: " + std::string{stackNonTerminal.getName()});
                    }
                    auto& currentParseTree = std::get<ImperfectParseTree>(*currentPtr);

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
                        return ParserRejectResult{"No production found for non-terminal: " + std::string{stackNonTerminal.getName()}, nextTokenIter->formatPosition()};
                    }

                    // Pop non-terminal
                    symbolStack.pop();

                    // Push symbols to temp stack and parse tree
                    LL1SymbolStack tempStack;
                    for (const auto& symbol : *production) {
                        if (std::holds_alternative<Terminal>(symbol)) {
                            const auto& terminalSymbol = std::get<Terminal>(symbol);
                            const auto& ptr = currentParseTree.addChild(terminalSymbol);
                            tempStack.push({terminalSymbol, ptr});
                        } else if (std::holds_alternative<NonTerminal>(symbol)) {
                            const auto& nonTerminalSymbol = std::get<NonTerminal>(symbol);
                            ImperfectParseTree childParseTree(nonTerminalSymbol);
                            const auto& ptr = currentParseTree.addChild(childParseTree);
                            tempStack.push({nonTerminalSymbol, ptr});
                        } else if (std::holds_alternative<char>(symbol)) {
                            const auto& eolSymbol = std::get<char>(symbol);
                            tempStack.push({eolSymbol, nullptr});
                        }
                        else {
                            throw std::runtime_error("Unknown symbol type in production");
                        }
                    }

                    // Push symbols to actual stack in reverse order
                    while (!tempStack.empty()) {
                        symbolStack.push(tempStack.top());
                        tempStack.pop();
                    }
                }
                else {
                    throw std::runtime_error("Unknown symbol type in parsing stack");
                }
            }

            return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter->formatPosition()};
        }
};
