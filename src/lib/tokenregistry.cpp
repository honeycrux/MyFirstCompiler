module;

#include <map>
#include <string_view>

export module tokenregistry;

export namespace TokenRegistry {

    constexpr int identifierId = 0;

    constexpr int integerLiteralId = 1;

    constexpr int floatLiteralId = 2;

    constexpr int stringLiteralId = 3;

    const std::map<std::string_view, int> keywordIdMap {
        {"int", 100},
        {"float", 101},
        {"str", 102},
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
        {"%", 211},
        {"&&", 212},
        {"||", 213},
        {"!", 214},
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

}
