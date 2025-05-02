module;

#include <string>
#include <map>
#include <vector>
#include <iostream>

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
        const int positionNumber;
        const std::string position;

    public:
        Token(int id, TokenType type, std::string_view value, int positionNumber, std::string position)
            : value(value), type(type), id(id), positionNumber(positionNumber), position(position) {}

        std::string toStringPrint() const {
            return "<" + value + ", " + std::string(tokenTypeNamesMap.at(type)) + ">";
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

        friend std::ostream& operator<<(std::ostream& os, const Token& token);

        static Token fromFile(std::istream& is) {
            int id;
            int typeId;
            std::string value;
            int positionNumber;
            std::string position;

            is >> typeId >> value >> positionNumber >> position;

            return Token(id, static_cast<TokenType>(typeId), value, positionNumber, position);
        }
};

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.getId() << " "
        << token.getType() << " "
        << token.getValue() << " "
        << token.getPositionNumber() << " "
        << token.getPosition();
    return os;
}
