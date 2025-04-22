module;

#include <map>
#include <optional>
#include <string_view>
#include <algorithm>

export module tokenregistry;

constexpr int identifierId = 0;

constexpr int numericLiteralId = 1;

constexpr int stringLiteralId = 2;

const std::map<std::string_view, int> keywordIdMap {
    {"int", 100},
    {"double", 101},
    {"string", 102},
    {"for", 103},
    {"if", 104},
    {"else", 105},
    {"return", 106},
    {"while", 107},
    {"do", 108},
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

export namespace TokenRegistry {
    constexpr int identifierId = ::identifierId;
    constexpr int numericLiteralId = ::numericLiteralId;
    constexpr int stringLiteralId = ::stringLiteralId;

    std::optional<int> getKeywordId(std::string_view str) {
        auto it = keywordIdMap.find(str);
        if (it == keywordIdMap.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<int> getOperatorId(std::string_view str) {
        auto it = operatorIdMap.find(str);
        if (it == operatorIdMap.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<int> getPunctuatorId(std::string_view str) {
        auto it = punctuatorIdMap.find(str);
        if (it == punctuatorIdMap.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    const int longestKeywordLength = computeLongestKeywordLength();
    const int longestOperatorLength = computeLongestOperatorLength();
    const int longestPunctuatorLength = computeLongestPunctuatorLength();
}
