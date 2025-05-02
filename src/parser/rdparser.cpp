module;

#include <vector>
#include <string>
#include <variant>
#include <map>
#include <stdexcept>

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
            auto productsIter = productMap.find(nonTerminal);
            if (productsIter == productMap.end()) {
                throw std::runtime_error("No production or subparser found for non-terminal: " + std::string{nonTerminal.getName()});
            }
            auto products = productsIter->second;

            std::vector<Token>::const_iterator bestIter = tokenIter;

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
                            auto rejectResult = std::get<ParserRejectResult>(result);
                            if (rejectResult.where->getPositionNumber() > bestIter->getPositionNumber()) {
                                bestIter = rejectResult.where;
                            }
                            success = false;
                            break;
                        }
                        auto acceptResult = std::get<ParserAcceptResult>(result);
                        parseTree.addChild(acceptResult.parseTree);
                        nextTokenIter = acceptResult.next;
                        if (acceptResult.bestIter->getPositionNumber() > bestIter->getPositionNumber()) {
                            bestIter = acceptResult.bestIter;
                        }
                    }
                    else if (std::holds_alternative<ParserBase*>(symbol)) {
                        const auto& subParser = std::get<ParserBase*>(symbol);
                        ParsingResult result = subParser->parse(nextTokenIter, tokenEnd);
                        if (std::holds_alternative<ParserRejectResult>(result)) {
                            auto rejectResult = std::get<ParserRejectResult>(result);
                            if (rejectResult.where->getPositionNumber() > bestIter->getPositionNumber()) {
                                bestIter = rejectResult.where;
                            }
                            success = false;
                            break;
                        }
                        auto acceptResult = std::get<ParserAcceptResult>(result);
                        parseTree.addChild(acceptResult.parseTree);
                        nextTokenIter = acceptResult.next;
                        if (acceptResult.bestIter->getPositionNumber() > bestIter->getPositionNumber()) {
                            bestIter = acceptResult.bestIter;
                        }
                    }
                    else {
                        throw std::runtime_error("Unknown symbol type");
                    }
                }
                if (success) {
                    return ParserAcceptResult{parseTree, nextTokenIter, bestIter};
                }
            }

            return ParserRejectResult{"Parsing error", bestIter};
        }

    public:
        RecursiveDescentParser(const NonTerminal& startSymbol, const RdpProductMap& productMap)
            : startSymbol(startSymbol), productMap(productMap) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            return parseNonTerminal(tokenIter, tokenEnd, startSymbol);
        }
};
