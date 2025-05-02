module;

#include <string>
#include <map>
#include <string_view>
#include <vector>
#include <algorithm>

export module token;

export enum TokenType {
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING,
    KEYWORD,
    OPERATOR,
    PUNCTUATOR,
};

const std::map<TokenType, std::string_view> tokenTypeNamesMap {
    {TokenType::IDENTIFIER, "identifier"},
    {TokenType::INTEGER, "integer"},
    {TokenType::FLOAT, "float"},
    {TokenType::STRING, "string"},
    {TokenType::KEYWORD, "keyword"},
    {TokenType::OPERATOR, "operator"},
    {TokenType::PUNCTUATOR, "punctuator"},
};

int calculatePosition(const std::string_view::const_iterator begin, const std::string_view::const_iterator where) {
    return std::distance(begin, where);
}

std::string formatPosition(const std::string_view::const_iterator begin, const std::string_view::const_iterator where) {
    const int line = std::count(begin, where, '\n') + 1;
    std::string_view::const_iterator lineStart = std::find_if(std::reverse_iterator<std::string_view::const_iterator>(where), std::reverse_iterator<std::string_view::const_iterator>(begin), [](char c) { return c == '\n'; }).base();
    const int column = std::distance(lineStart, where) + 1;
    const std::string position = std::to_string(line) + ":" + std::to_string(column);
    return position;
}

export class Token {
    private:
        const int id;
        const TokenType type;
        const std::string value;
        const int positionNumber;
        const std::string position;

    public:
        Token(int id, TokenType type, std::string_view value, std::string_view::const_iterator begin, std::string_view::const_iterator where)
            : value(value), type(type), id(id), positionNumber(calculatePosition(begin, where)), position(formatPosition(begin, where)) {}

        std::string toStringPrint() const {
            return "<" + value + ", " + std::string(tokenTypeNamesMap.at(type)) + ">";
        }

        std::string toStringWrite() const {
            return "<" + value + ", " + std::to_string(id) + ">";
        }

        int getId() const {
            return id;
        }

        TokenType getType() const {
            return type;
        }

        std::string getValue() const {
            return value;
        }

        int getPositionNumber() const {
            return positionNumber;
        }

        std::string getPosition() const {
            return position;
        }
};
