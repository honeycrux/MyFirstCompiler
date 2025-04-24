module;

#include <string>
#include <vector>
#include <variant>

export module parserbase;

import token;

export struct AcceptResult {
    std::vector<Token>::const_iterator next;
};

export struct RejectResult {
    std::string message;
    std::vector<Token>::const_iterator where;
};

export using ParsingResult = std::variant<AcceptResult, RejectResult>;

export class Parser {

    public:
        Parser() {}

        virtual ParsingResult parse(std::vector<Token>::const_iterator tokenIter, const std::vector<Token>::const_iterator tokenEnd) const = 0;
};

export class Terminal {
    private:
        const int id;

    public:
        Terminal(int id) : id(id) {}
        Terminal(const Token& token) : id(token.getId()) {}

        int getId() const {
            return id;
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

export using Production = std::pair<NonTerminal, std::vector<Symbol>>;
