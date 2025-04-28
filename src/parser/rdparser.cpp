module;

#include <vector>
#include <string>
#include <variant>
#include <map>
#include <stdexcept>
// #include <iostream>

export module rdparser;

import token;
import symbol;
import parserbase;

export using RdpProduct = std::vector<std::variant<NonTerminal, Terminal, ParserBase*>>;
export using RdpProductMap = std::map<NonTerminal, std::vector<RdpProduct>>;

export class RecursiveDescentParser : public ParserBase {
    private:
        const NonTerminal startSymbol;
        const RdpProductMap productMap;

        ParsingResult parseNonTerminal(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd, const NonTerminal& nonTerminal) const {
            // std::cout << "[in] " << nonTerminal.getName() << " this: " << (tokenIter == tokenEnd ? "EOL" : tokenIter->toStringPrint()) << std::endl;

            auto productsIter = productMap.find(nonTerminal);
            if (productsIter == productMap.end()) {
                throw std::runtime_error("No production or subparser found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            auto products = productsIter->second;

            for (const auto& product : products) {
                ParseTree parseTree(nonTerminal);
                auto nextTokenIter = tokenIter;
                bool success = true;
                for (const auto& symbol : product) {
                    if (std::holds_alternative<Terminal>(symbol)) {
                        const auto& terminalSymbol = std::get<Terminal>(symbol);
                        if (nextTokenIter == tokenEnd || !terminalSymbol.matchesToken(*nextTokenIter)) {
                            success = false;
                            break;
                        }
                        parseTree.addChild(*nextTokenIter);
                        nextTokenIter++;
                    }
                    else if (std::holds_alternative<NonTerminal>(symbol)) {
                        const auto& nonTerminalSymbol = std::get<NonTerminal>(symbol);
                        ParsingResult result = parseNonTerminal(nextTokenIter, tokenEnd, nonTerminalSymbol);
                        if (std::holds_alternative<ParserRejectResult>(result)) {
                            success = false;
                            break;
                        }
                        auto acceptResult = std::get<ParserAcceptResult>(result);
                        parseTree.addChild(acceptResult.parseTree);
                        nextTokenIter = acceptResult.next;
                    }
                    else if (std::holds_alternative<ParserBase*>(symbol)) {
                        const auto& subParser = std::get<ParserBase*>(symbol);
                        ParsingResult result = subParser->parse(nextTokenIter, tokenEnd);
                        if (std::holds_alternative<ParserRejectResult>(result)) {
                            success = false;
                            break;
                        }
                        auto acceptResult = std::get<ParserAcceptResult>(result);
                        parseTree.addChild(acceptResult.parseTree);
                        nextTokenIter = acceptResult.next;
                    }
                    else {
                        throw std::runtime_error("Unknown symbol type");
                    }
                }
                if (success) {
                    // std::cout << "[success] " << nonTerminal.getName() << " next: " << (nextTokenIter == tokenEnd ? "EOL" : nextTokenIter->toStringPrint()) << std::endl;
                    return ParserAcceptResult{parseTree, nextTokenIter};
                }
            }
            // std::cout << "[fail] " << nonTerminal.getName() << std::endl;
            return ParserRejectResult{"No valid production found for non-terminal: " + std::string{nonTerminal.getName()}, tokenIter->formatPosition()};
        }

    public:
        RecursiveDescentParser(const NonTerminal& startSymbol, const RdpProductMap& productMap)
            : startSymbol(startSymbol), productMap(productMap) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            return parseNonTerminal(tokenIter, tokenEnd, startSymbol);
        }
};
