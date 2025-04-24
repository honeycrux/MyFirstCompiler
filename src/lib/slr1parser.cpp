module;

#include <string>
#include <variant>
#include <vector>
#include <map>
#include <stack>
#include <optional>

export module sr1parser;

import token;
import parserbase;

class State {
    private:
        std::string name;

    public:
        State(const std::string& name) : name(name) {}

        std::string_view getName() const {
            return name;
        }

        bool operator==(const State& other) const {
            return name == other.name;
        }

        std::strong_ordering operator<=>(const State& other) const {
            return name <=> other.name;
        }
};

constexpr int ACCEPT = 0;

using Instruction = std::variant<State, Production, int>;

using ParsingTable = std::map<std::pair<State, Symbol>, Instruction>;

class SLR1Parser : public Parser {
    private:
        const State startState;
        const ParsingTable parsingTable;

    public:
        SLR1Parser(const State startState, const ParsingTable& parsingTable)
            : startState(startState), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            State currentState = startState;
            std::stack<Symbol> symbolStack;

            while (true) {
                if (tokenIter == tokenEnd) {
                    return RejectResult{"Unexpected end of input", tokenIter};
                }

                const auto currentSymbol = symbolStack.top();

                const auto instructionIter = parsingTable.find(std::make_pair(currentState, currentSymbol));
                if (instructionIter == parsingTable.end()) {
                    return RejectResult{"Unexpected token: " + (*tokenIter).toStringPrint(), tokenIter};
                }
                const auto& instruction = instructionIter->second;

                if (std::holds_alternative<State>(instruction)) {
                    // Shift

                    currentState = std::get<State>(instruction);
                    tokenIter++;
                } else if (std::holds_alternative<Production>(instruction)) {
                    // Reduce

                    const auto& production = std::get<Production>(instruction);
                    const auto& nonTerminal = production.first;
                    const auto& symbols = production.second;

                    // Pop symbols from stack
                    for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
                        symbolStack.pop();
                    }

                    // Push non-terminal
                    symbolStack.push(nonTerminal);
                } else if (std::holds_alternative<int>(instruction)) {
                    // Accept

                    return AcceptResult{tokenIter};
                }
            }
        }
};
