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

using ParsingTable = std::map<std::pair<NonTerminal, Terminal>, std::vector<Symbol>>;

export class LL1Parser : public Parser {
    private:
        const NonTerminal startSymbol;
        const ParsingTable parsingTable;

    public:
        LL1Parser(const NonTerminal startSymbol, const ParsingTable& parsingTable)
            : startSymbol(startSymbol), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            auto currentTokenIter = tokenIter;
            std::stack<Symbol> symbolStack;

            symbolStack.push(startSymbol);

            while (! (symbolStack.empty()) || currentTokenIter != tokenEnd) {
                if (symbolStack.empty()) {
                    return RejectResult{"Unexpected end of symbol stack", currentTokenIter};
                }

                if (currentTokenIter == tokenEnd) {
                    return RejectResult{"Unexpected end of input", currentTokenIter};
                }

                const auto& currentSymbol = symbolStack.top();

                if (std::holds_alternative<Terminal>(currentSymbol)) {
                    // The top of symbol stack is a terminal
                    // Check if it matches current token

                    const auto& stackTerminal = std::get<Terminal>(currentSymbol);
                    if (stackTerminal.matchesToken(*currentTokenIter)) {
                        symbolStack.pop();
                        currentTokenIter++;
                    } else {
                        return RejectResult{"Unexpected token: " + currentTokenIter->toStringPrint(), currentTokenIter };
                    }
                } else if (std::holds_alternative<NonTerminal>(currentSymbol)) {
                    // The top of symbol stack is a non-terminal
                    // Look up parsing table for production rule

                    const auto& stackNonTerminal = std::get<NonTerminal>(currentSymbol);
                    const auto& currentTokenAsTerminal = Terminal{*currentTokenIter};
                    const auto productionIter = parsingTable.find(std::make_pair(stackNonTerminal, currentTokenAsTerminal));
                    if (productionIter == parsingTable.end()) {
                        return RejectResult{"Unexpected token: " + currentTokenIter->toStringPrint(), currentTokenIter};
                    }
                    const auto& production = productionIter->second;
                    symbolStack.pop();
                    for (const auto& symbol : production) {
                        symbolStack.push(symbol);
                    }
                }
                else {
                    throw std::runtime_error("Unknown symbol type in parsing stack");
                }
            }

            return AcceptResult{currentTokenIter};
        }
};
