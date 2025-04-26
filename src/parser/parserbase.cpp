module;

#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include <algorithm>

export module parserbase;

import token;

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

export class Terminal {
    private:
        const int id;
        const std::string name;

    public:
        Terminal(int id, std::string_view name) : id(id), name(name) {}

        std::string_view getName() const {
            return name;
        }

        bool matchesToken(const Token& token) const {
            return token.getId() == id;
        }

        std::strong_ordering operator<=>(const Terminal& other) const {
            return id <=> other.id;
        }
};

export class NonTerminal {
    private:
        const std::string name;

    public:
        NonTerminal(const std::string& name) : name(name) {}

        std::string_view getName() const {
            return name;
        }

        bool operator==(const NonTerminal& other) const {
            return name == other.name;
        }

        std::strong_ordering operator<=>(const NonTerminal& other) const {
            return name <=> other.name;
        }
};

export using Symbol = std::variant<Terminal, NonTerminal>;

export using Product = std::vector<Symbol>;

export using Production = std::pair<NonTerminal, Product>;

// export class ParseTree {
//     private:
//         const NonTerminal symbol;
//         const std::vector<std::variant<Terminal, ParseTree>> children;

//     public:
//         ParseTree(const Symbol& symbol) : symbol(symbol) {}

//         void toAst() const {}

//         void print() const {
//             const auto visitor = overloads{
//                 [](const Terminal& terminal) {
//                     std::cout << terminal.getName() << ", ";
//                 },
//                 [](const ParseTree& parseTree) {
//                     parseTree.print();
//                 }
//             };
//             std::cout << symbol.getName() << "( ";
//             for (auto child = children.begin(); child != children.end(); ++child) {
//                 if (child != children.begin()) {
//                     std::cout << ", ";
//                 }
//                 std::visit(visitor, *child);
//             }
//             std::cout << " )";
//         }
// };

export struct ParserAcceptResult {
    // ParseTree parseTree;
    std::vector<Token>::const_iterator next;
};

export struct ParserRejectResult {
    std::string message;
    std::vector<Token>::const_iterator where;
};

export using ParsingResult = std::variant<ParserAcceptResult, ParserRejectResult>;

export class ParserBase {
    public:
        ParserBase() {}
        virtual ~ParserBase() = default;
        virtual ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const = 0;
};
