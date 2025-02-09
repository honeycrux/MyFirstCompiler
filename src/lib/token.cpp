module;

#include <string>
#include <map>
#include <string_view>

export module token;

export enum TokenType {
    IDENTIFIER,
    NUMBER,
    STRING,
    KEYWORD,
    OPERATOR,
    PUNCTUATOR,
};

const std::map<TokenType, std::string_view> tokenTypeNamesMap {
    {TokenType::IDENTIFIER, "identifier"},
    {TokenType::NUMBER, "number"},
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
    public:
        Token(std::string_view value, TokenType type, int id) : value(value), type(type), id(id) {}
        std::string toStringPrint() const {
            return "<" + value + ", " + std::string(tokenTypeNamesMap.at(type)) + ">";
        }
        std::string toStringWrite() const {
            return "<" + value + ", " + std::to_string(id) + ">";
        }
};
