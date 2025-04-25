module;

#include <vector>
#include <string>
#include <variant>
#include <map>
#include <stdexcept>

export module rdparser;

import token;
import parserbase;

using RdpProduct = std::vector<std::variant<NonTerminal, Terminal, ParserBase*>>;
using RdpProductMap = std::map<NonTerminal, std::vector<RdpProduct>>;

export class RecursiveDescentParser : public ParserBase {
    private:
        const NonTerminal startSymbol;
        const RdpProductMap productMap;

        ParsingResult parseNonTerminal(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd, const NonTerminal& nonTerminal) const {
            auto productsIter = productMap.find(nonTerminal);
            if (productsIter == productMap.end()) {
                throw std::runtime_error("No production or subparser found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            auto products = productsIter->second;

            for (const auto& product : products) {
                auto currentTokenIter = tokenIter;
                bool success = true;
                for (const auto& symbol : product) {
                    if (std::holds_alternative<Terminal>(symbol)) {
                        const auto& terminalSymbol = std::get<Terminal>(symbol);
                        if (currentTokenIter == tokenEnd || !terminalSymbol.matchesToken(*currentTokenIter)) {
                            success = false;
                            break;
                        }
                        currentTokenIter++;
                    }
                    else if (std::holds_alternative<NonTerminal>(symbol)) {
                        const auto& nonTerminalSymbol = std::get<NonTerminal>(symbol);
                        ParsingResult result = parseNonTerminal(currentTokenIter, tokenEnd, nonTerminalSymbol);
                        if (std::holds_alternative<RejectResult>(result)) {
                            success = false;
                            break;
                        }
                        currentTokenIter = std::get<AcceptResult>(result).next;
                    }
                    else if (std::holds_alternative<ParserBase*>(symbol)) {
                        const auto& subParser = std::get<ParserBase*>(symbol);
                        ParsingResult result = subParser->parse(currentTokenIter, tokenEnd);
                        if (std::holds_alternative<RejectResult>(result)) {
                            success = false;
                            break;
                        }
                        currentTokenIter = std::get<AcceptResult>(result).next;
                    }
                    else {
                        throw std::runtime_error("Unknown symbol type");
                    }
                }
                if (success) {
                    return AcceptResult{currentTokenIter};
                }
            }
            return RejectResult{"No valid production found for non-terminal: " + std::string{nonTerminal.getName()}, tokenIter};
        }

    public:
        RecursiveDescentParser(const NonTerminal& startSymbol, const RdpProductMap& productMap)
            : startSymbol(startSymbol), productMap(productMap) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            return parseNonTerminal(tokenIter, tokenEnd, startSymbol);
        }
};
