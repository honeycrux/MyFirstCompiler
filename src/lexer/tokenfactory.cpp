module;

#include <optional>
#include <string_view>
#include <algorithm>

export module tokenfactory;

import token;
import tokenregistry;

int computeLongestKeywordLength() {
    auto it = std::max_element(TokenRegistry::keywordIdMap.begin(), TokenRegistry::keywordIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

int computeLongestOperatorLength() {
    auto it = std::max_element(TokenRegistry::operatorIdMap.begin(), TokenRegistry::operatorIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

int computeLongestPunctuatorLength() {
    auto it = std::max_element(TokenRegistry::punctuatorIdMap.begin(), TokenRegistry::punctuatorIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

export namespace TokenFactory {
    Token getIdentifierToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        return Token(TokenRegistry::identifierId, TokenType::IDENTIFIER, str, begin, where);
    }

    Token getIntegerLiteralToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        return Token(TokenRegistry::integerLiteralId, TokenType::INTEGER, str, begin, where);
    }

    Token getFloatLiteralToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        return Token(TokenRegistry::floatLiteralId, TokenType::FLOAT, str, begin, where);
    }

    Token getStringLiteralToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        return Token(TokenRegistry::stringLiteralId, TokenType::STRING, str, begin, where);
    }

    std::optional<Token> findKeywordToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        auto it = TokenRegistry::keywordIdMap.find(str);
        if (it == TokenRegistry::keywordIdMap.end()) {
            return std::nullopt;
        }
        return Token(it->second, TokenType::KEYWORD, str, begin, where);
    }

    std::optional<Token> findOperatorToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        auto it = TokenRegistry::operatorIdMap.find(str);
        if (it == TokenRegistry::operatorIdMap.end()) {
            return std::nullopt;
        }
        return Token(it->second, TokenType::OPERATOR, str, begin, where);
    }

    std::optional<Token> findPunctuatorToken(std::string_view str, std::string_view::const_iterator begin, std::string_view::const_iterator where) {
        auto it = TokenRegistry::punctuatorIdMap.find(str);
        if (it == TokenRegistry::punctuatorIdMap.end()) {
            return std::nullopt;
        }
        return Token(it->second, TokenType::PUNCTUATOR, str, begin, where);
    }

    const int longestKeywordLength = computeLongestKeywordLength();
    const int longestOperatorLength = computeLongestOperatorLength();
    const int longestPunctuatorLength = computeLongestPunctuatorLength();
}
