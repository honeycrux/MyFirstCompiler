module;

#include <iostream>
#include <string>
#include <map>
#include <optional>
#include <string_view>
#include <algorithm>

export module tokenfactory;

import token;

const int identifierId = 0;

const int numericLiteralId = 1;

const int stringLiteralId = 2;

const std::map<std::string_view, int> keywordIdMap {
    {"int", 100},
    {"main", 101},
    {"char", 102},
    {"for", 103},
    {"if", 104},
    {"else", 105},
    {"return", 106},
};

const std::map<std::string_view, int> operatorIdMap {
    {"=", 200},
    {"==", 201},
    {"!=", 202},
    {">=", 203},
    {"<=", 204},
    {">", 205},
    {"<", 206},
    {"+", 207},
    {"-", 208},
    {"*", 209},
    {"/", 210},
};

const std::map<std::string_view, int> punctuatorIdMap {
    {"{", 300},
    {"}", 301},
    {",", 302},
    {";", 303},
    {"(", 304},
    {")", 305},
    {"[", 306},
    {"]", 307},
};

int computeLongestKeywordLength() {
    auto it = std::max_element(keywordIdMap.begin(), keywordIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

int computeLongestOperatorLength() {
    auto it = std::max_element(operatorIdMap.begin(), operatorIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

int computeLongestPunctuatorLength() {
    auto it = std::max_element(punctuatorIdMap.begin(), punctuatorIdMap.end(), [](const auto& a, const auto& b) {
        return a.first.size() < b.first.size();
    });
    return it->first.size();
}

export namespace TokenFactory {
    const int longestKeywordLength = computeLongestKeywordLength();
    const int longestOperatorLength = computeLongestOperatorLength();
    const int longestPunctuatorLength = computeLongestPunctuatorLength();

    std::optional<Token> getIdentifierToken(std::string_view str) {
        // 1. First character is alpha
        if (str.empty()) {
            return std::nullopt;
        }
        if (!(std::isalpha(str[0]) || str[0] == '_')) {
            return std::nullopt;
        }
        // 2. All characters are alnum or underscore
        for (char c : str) {
            if (!(std::isalnum(c) || c == '_')) {
                return std::nullopt;
            }
        }
        return std::make_optional<Token>(str, TokenType::IDENTIFIER, identifierId);
    }

    std::optional<Token> getNumberToken(std::string_view str) {
        // 1. Starts with a digit
        if (str.empty()) {
            return std::nullopt;
        }
        if (!std::isdigit(str[0])) {
            return std::nullopt;
        }
        // 2. All characters are digits or dot
        for (char c : str) {
            if (!(std::isdigit(c) || c == '.')) {
                return std::nullopt;
            }
        }
        // 3. Ends with a digit
        if (!std::isdigit(str.back())) {
            return std::nullopt;
        }
        // 4. Contains at most one dot
        if (std::count(str.begin(), str.end(), '.') > 1) {
            return std::nullopt;
        }
        // Done
        return std::make_optional<Token>(str, TokenType::NUMBER, numericLiteralId);
    }

    std::optional<Token> getStringToken(std::string_view str) {
        // 1. Starts with an apostrophe
        if (str.empty()) {
            return std::nullopt;
        }
        if (str[0] != '"') {
            return std::nullopt;
        }
        // 2. No apostrophe character in the string
        enum State {
            NORMAL,
            ESCAPE,
        };
        State state = NORMAL;
        for (char c : str.substr(1, str.size() - 2)) {
            if (state == NORMAL && c == '"') {
                return std::nullopt;
            }
            // Decide whether the *next* character is an escape character
            if (state == NORMAL) {
                if (c == '\\') {
                    state = ESCAPE;
                }
            } else if (state == ESCAPE) {
                state = NORMAL;
            }
        }
        // 3. Ends with an apostrophe
        State endingState = state;
        if (!(endingState == NORMAL && str.back() == '"')) {
            return std::nullopt;
        }
        // Done
        return std::make_optional<Token>(str, TokenType::STRING, stringLiteralId);
    }

    std::optional<Token> getKeywordToken(std::string_view str) {
        auto it = keywordIdMap.find(str);
        if (it == keywordIdMap.end()) {
            return std::nullopt;
        }
        return std::make_optional<Token>(str, TokenType::KEYWORD, it->second);
    }

    std::optional<Token> getOperatorToken(std::string_view str) {
        auto it = operatorIdMap.find(str);
        if (it == operatorIdMap.end()) {
            return std::nullopt;
        }
        return std::make_optional<Token>(str, TokenType::OPERATOR, it->second);
    }

    std::optional<Token> getPunctuatorToken(std::string_view str) {
        auto it = punctuatorIdMap.find(str);
        if (it == punctuatorIdMap.end()) {
            return std::nullopt;
        }
        return std::make_optional<Token>(str, TokenType::PUNCTUATOR, it->second);
    }
};
