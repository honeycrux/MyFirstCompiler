module;

#include <string>
#include <vector>
#include <variant>

export module symbol;

import token;

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

export using TerminalOrEOL = std::variant<Terminal, char>;

export using SymbolOrEOL = std::variant<Terminal, NonTerminal, char>;

export using Product = std::vector<Symbol>;

export using Production = std::pair<NonTerminal, Product>;

export constexpr char EOL = 0x1E;
