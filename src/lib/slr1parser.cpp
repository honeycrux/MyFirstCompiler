module;

#include <string>
#include <variant>
#include <vector>
#include <map>
#include <stack>
#include <optional>
#include <stdexcept>

export module slr1parser;

import token;
import parserbase;
import terminalfactory;

export class State {
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

export constexpr int ACCEPT = 0;

export using Instruction = std::variant<State, Production, int>;

export using SLR1ParsingTable = std::map<std::pair<State, std::optional<Symbol>>, Instruction>;

export class SLR1Parser : public ParserBase {
    private:
        const State startState;
        const SLR1ParsingTable parsingTable;

    public:
        SLR1Parser(const State startState, const SLR1ParsingTable& parsingTable)
            : startState(startState), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            std::stack<std::pair<State, std::optional<Symbol>>> stateSymbolStack;

            while (true) {
                if (tokenIter == tokenEnd) {
                    return RejectResult{"Unexpected end of input", tokenIter};
                }

                const auto getCurrentState = [&]() {
                    if (stateSymbolStack.empty()) {
                        return startState;
                    } else {
                        return stateSymbolStack.top().first;
                    }
                };

                std::optional<Symbol> nextSymbol = std::nullopt;
                if (tokenIter + 1 != tokenEnd) {
                    nextSymbol.emplace(TerminalFactory::fromToken(*tokenIter));
                }

                const auto instructionIter = parsingTable.find(std::make_pair(getCurrentState(), nextSymbol));
                if (instructionIter == parsingTable.end()) {
                    return RejectResult{"Unexpected token: " + (*tokenIter).toStringPrint(), tokenIter};
                }
                const auto& instruction = instructionIter->second;

                if (std::holds_alternative<State>(instruction)) {
                    // Shift

                    // Push new state to stack
                    const auto newState = std::get<State>(instruction);
                    stateSymbolStack.push(std::make_pair(newState, TerminalFactory::fromToken(*(tokenIter + 1))));
                    tokenIter++;
                } else if (std::holds_alternative<Production>(instruction)) {
                    // Reduce

                    const auto& production = std::get<Production>(instruction);
                    const auto& nonTerminal = production.first;
                    const auto& symbols = production.second;

                    // Pop symbols from stack
                    for (int i = 0; i < symbols.size(); i++) {
                        stateSymbolStack.pop();
                    }

                    // Find new state
                    const auto nextStateIter = parsingTable.find(std::make_pair(getCurrentState(), std::optional<Symbol>{nonTerminal}));
                    if (nextStateIter == parsingTable.end()) {
                        return RejectResult{"Unexpected token: " + (*tokenIter).toStringPrint(), tokenIter};
                    }
                    const auto& nextStateInstruction = nextStateIter->second;
                    if (!std::holds_alternative<State>(nextStateInstruction)) {
                        return RejectResult{"Unexpected token: " + (*tokenIter).toStringPrint(), tokenIter};
                    }
                    const auto newState = std::get<State>(nextStateInstruction);

                    // Push new state to stack
                    stateSymbolStack.push(std::make_pair(newState, nonTerminal));
                } else if (std::holds_alternative<int>(instruction)) {
                    // Accept

                    return AcceptResult{tokenIter};
                } else {
                    throw std::runtime_error("Unknown instruction type");
                }
            }
        }
};
