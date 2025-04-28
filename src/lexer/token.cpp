module;

#include <string>
#include <map>
#include <string_view>
#include <vector>

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

export class Token {
    private:
        const int id;
        const TokenType type;
        const std::string value;
        const std::string_view::const_iterator begin;
        const std::string_view::const_iterator where;
    public:
        Token(int id, TokenType type, std::string_view value, std::string_view::const_iterator begin, std::string_view::const_iterator where)
            : value(value), type(type), id(id), begin(begin), where(where) {}
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
        std::string formatPosition() const {
            return "position: " + std::to_string(std::distance(begin, where) + 1);
        }
};
