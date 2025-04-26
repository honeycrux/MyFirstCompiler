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
import parserbase;
import terminalfactory;

export using LL1ParsingTable = std::map<std::pair<NonTerminal, std::optional<Terminal>>, std::vector<std::optional<Symbol>>>;

export class LL1Parser : public ParserBase {
    private:
        const NonTerminal startSymbol;
        const LL1ParsingTable parsingTable;

    public:
        LL1Parser(const NonTerminal startSymbol, const LL1ParsingTable& parsingTable)
            : startSymbol(startSymbol), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            auto nextTokenIter = tokenIter;
            std::stack<std::optional<Symbol>> symbolStack;
            bool assumeEndOfLine = false;

            symbolStack.push(startSymbol);

            while (true) {
                if (symbolStack.empty()) {
                    return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter};
                }

                if (nextTokenIter == tokenEnd) {
                    return ParserRejectResult{"Unexpected end of input", nextTokenIter};
                }

                const auto& currentSymbol = symbolStack.top();

                if (!currentSymbol.has_value()) {
                    // The top of symbol stack is EOL

                    symbolStack.pop();
                    return ParserAcceptResult{nextTokenIter};
                }

                if (std::holds_alternative<Terminal>(currentSymbol.value())) {
                    // The top of symbol stack is a terminal
                    // Check if it matches current token

                    const auto& stackTerminal = std::get<Terminal>(currentSymbol.value());
                    if (stackTerminal.matchesToken(*nextTokenIter)) {
                        symbolStack.pop();
                        nextTokenIter++;
                    } else {
                        return ParserRejectResult{"Unexpected token: " + nextTokenIter->toStringPrint(), nextTokenIter };
                    }
                } else if (std::holds_alternative<NonTerminal>(currentSymbol.value())) {
                    // The top of symbol stack is a non-terminal
                    // Look up parsing table for production rule

                    const auto& stackNonTerminal = std::get<NonTerminal>(currentSymbol.value());
                    const auto findProduction = [&]()->std::optional<std::vector<std::optional<Symbol>>> {
                        if (!assumeEndOfLine && nextTokenIter != tokenEnd) {
                            const auto& currentTokenAsTerminal = TerminalFactory::fromToken(*nextTokenIter);
                            const auto productionIter = parsingTable.find(std::make_pair(stackNonTerminal, currentTokenAsTerminal));
                            if (productionIter != parsingTable.end()) {
                                const auto& production = productionIter->second;
                                return production;
                            }
                        }
                        assumeEndOfLine = true;
                        const auto productionIter = parsingTable.find(std::make_pair(stackNonTerminal, std::nullopt));
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
                    symbolStack.pop();
                    for (auto symbolIter = production->rbegin(); symbolIter != production->rend(); ++symbolIter) {
                        symbolStack.push(*symbolIter);
                    }
                }
                else {
                    throw std::runtime_error("Unknown symbol type in parsing stack");
                }
            }

            return ParserRejectResult{"Unexpected end of symbol stack", nextTokenIter};
        }
};
