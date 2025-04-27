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
import symbol;
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

export constexpr char ACCEPT = 0;

export using Instruction = std::variant<State, int, char>;

export using ProductionMap = std::map<int, Production>;

export using SLR1ParsingTable = std::map<std::pair<State, SymbolOrEOL>, Instruction>;

export class SLR1Parser : public ParserBase {
    private:
        const State startState;
        const ProductionMap productionMap;
        const SLR1ParsingTable parsingTable;

    public:
        SLR1Parser(const State startState, const ProductionMap productionMap, const SLR1ParsingTable& parsingTable)
            : startState(startState), productionMap(productionMap), parsingTable(parsingTable) {}

        ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const override {
            auto nextTokenIter = tokenIter;
            bool assumeEndOfLine = false;
            std::stack<std::pair<State, std::variant<Token, ParseTree>>> stateSymbolStack;

            while (true) {
                const auto getCurrentState = [&]() {
                    if (stateSymbolStack.empty()) {
                        return startState;
                    } else {
                        return stateSymbolStack.top().first;
                    }
                };

                const auto findInstruction = [&]()->std::optional<Instruction> {
                    if (!assumeEndOfLine && nextTokenIter != tokenEnd) {
                        const auto& currentTokenAsTerminal = TerminalFactory::fromToken(*nextTokenIter);
                        const auto instructionIter = parsingTable.find(std::make_pair(getCurrentState(), currentTokenAsTerminal));
                        if (instructionIter != parsingTable.end()) {
                            return instructionIter->second;
                        }
                    }
                    assumeEndOfLine = true;
                    const auto instructionIter = parsingTable.find(std::make_pair(getCurrentState(), EOL));
                    if (instructionIter != parsingTable.end()) {
                        return instructionIter->second;
                    }
                    return std::nullopt;
                };

                const auto instructionIter = findInstruction();
                if (!instructionIter.has_value()) {
                    return ParserRejectResult{"No production found", nextTokenIter};
                }
                const auto& instruction = instructionIter.value();

                if (std::holds_alternative<State>(instruction)) {
                    // Shift

                    // Push new state to stack
                    const auto newState = std::get<State>(instruction);
                    if (nextTokenIter == tokenEnd) {
                        return ParserRejectResult{"Unexpected end of input", nextTokenIter};
                    }
                    stateSymbolStack.push(std::make_pair(newState, *nextTokenIter));
                    nextTokenIter++;
                } else if (std::holds_alternative<int>(instruction)) {
                    // Reduce

                    const auto& productionId = std::get<int>(instruction);
                    const auto productionIter = productionMap.find(productionId);
                    if (productionIter == productionMap.end()) {
                        throw std::runtime_error("No production found for ID: " + std::to_string(productionId));
                    }
                    const auto& nonTerminal = productionIter->second.first;
                    const auto& symbols = productionIter->second.second;

                    ParseTree newParseTree{nonTerminal};

                    // Pop symbols from stack
                    for (int i = 0; i < symbols.size(); i++) {
                        const auto& symbol = stateSymbolStack.top().second;
                        newParseTree.addChild(symbol);
                        stateSymbolStack.pop();
                    }

                    // Find new state
                    const auto nextStateIter = parsingTable.find(std::make_pair(getCurrentState(), SymbolOrEOL{nonTerminal}));
                    if (nextStateIter == parsingTable.end()) {
                        return ParserRejectResult{"Unexpected token: " + (*nextTokenIter).toStringPrint(), nextTokenIter};
                    }
                    const auto& nextStateInstruction = nextStateIter->second;
                    if (!std::holds_alternative<State>(nextStateInstruction)) {
                        return ParserRejectResult{"Unexpected token: " + (*nextTokenIter).toStringPrint(), nextTokenIter};
                    }
                    const auto newState = std::get<State>(nextStateInstruction);

                    // Push new state to stack
                    stateSymbolStack.push(std::make_pair(newState, newParseTree));
                } else if (std::holds_alternative<char>(instruction)) {
                    // Accept

                    const auto& stackTop = stateSymbolStack.top().second;
                    if (std::holds_alternative<Token>(stackTop)) {
                        return ParserRejectResult{"Unexpected token: " + std::get<Token>(stackTop).toStringPrint(), nextTokenIter};
                    }
                    const auto& parseTree = std::get<ParseTree>(stackTop);

                    return ParserAcceptResult{parseTree, nextTokenIter};
                } else {
                    throw std::runtime_error("Unknown instruction type");
                }
            }
        }
};
