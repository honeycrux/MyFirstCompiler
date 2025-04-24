module;

#include <cassert>
#include <vector>
#include <string>
#include <string_view>
#include <variant>

export module rdparser;

import token;
import parserbase;

class RdpSymbol {
    public:
        RdpSymbol() {}

        virtual ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const = 0;
};

using RdpProduct = std::vector<RdpSymbol*>;

export class RdpTerminal : public RdpSymbol {
    private:
        const Terminal terminal;

    public:
        RdpTerminal(const Terminal& terminal) : terminal(terminal) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            if (tokenIter == tokenEnd || terminal.matchesToken(*tokenIter)) {
                return RejectResult{"Unexpected token", tokenIter};
            }
            return AcceptResult{++tokenIter};
        }
};

export class RdpNonTerminal : public RdpSymbol {
    private:
        const std::string name;
        const std::vector<RdpProduct> rdpProducts;

    public:
        RdpNonTerminal(const std::string& name, const std::vector<RdpProduct>& rdpProducts) : name(name), rdpProducts(rdpProducts) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            for (const auto& product : rdpProducts) {
                auto currentTokenIter = tokenIter;
                bool success = true;
                for (const auto& symbol : product) {
                    ParsingResult result = symbol->parse(currentTokenIter, tokenEnd);
                    if (std::holds_alternative<AcceptResult>(result)) {
                        const auto& acceptResult = std::get<AcceptResult>(result);
                        currentTokenIter = acceptResult.next;
                    } else {
                        success = false;
                        break;
                    }
                }
                if (success) {
                    return AcceptResult{currentTokenIter};
                }
            }
            return RejectResult{"Failed to parse " + name, tokenIter};
        }
};

export class RdpSubparser : public RdpSymbol {
    private:
        const Parser* parser;

    public:
        RdpSubparser(const Parser* parser) : parser(parser) {
            assert(parser != nullptr);
        }

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            return parser->parse(tokenIter, tokenEnd);
        }
};

export class RecursiveDescentParser : public Parser {
    private:
        const RdpNonTerminal startSymbol;

    public:
        RecursiveDescentParser(const RdpNonTerminal& startSymbol) : startSymbol(startSymbol) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            ParsingResult result = startSymbol.parse(tokenIter, tokenEnd);
            if (std::holds_alternative<AcceptResult>(result)) {
                return std::get<AcceptResult>(result);
            } else {
                // const auto& rejectResult = std::get<RejectResult>(result);
                // std::cerr << "Parsing error: " << rejectResult.message << " at token " << *rejectResult.where << std::endl;
            }
            return RejectResult{"Parsing error", tokenIter};
        }
};
