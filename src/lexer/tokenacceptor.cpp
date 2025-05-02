module;

#include <string>
#include <memory>
#include <cctype>
#include <optional>
#include <variant>

export module tokenacceptor;

import token;
import tokenfactory;

export struct TokenAcceptResult {
    Token token;
    std::string_view::const_iterator next;
};

export struct TokenRejectResult {
    std::string message;
    std::string_view::const_iterator where;
};

using TokenAcceptorResult = std::variant<TokenAcceptResult, TokenRejectResult>;

bool nextCharacterIsConflicting(char nextChar) {
    // A conflicting token includes an identifier, a number, a string, or a keyword
    // Two conflicting tokens, even of the same type, cannot be adjacent
    // This function identifies if the next character starts a conflicting token
    return std::isalnum(nextChar) || nextChar == '_' || nextChar == '"';
}

export class TokenAcceptor {
    public:
        TokenAcceptor() {}
        virtual ~TokenAcceptor() = default;
        virtual TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) = 0;
};

export class IdentifierAcceptor : public TokenAcceptor {
    public:
        IdentifierAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            if (stringStart == stringEnd || !(std::isalpha(*stringStart) || *stringStart == '_')) {
                return TokenRejectResult{"Not an idenfier", stringStart};
            }
            std::string value;
            while (stringIter != stringEnd && (std::isalnum(*stringIter) || *stringIter == '_')) {
                value += *stringIter;
                stringIter++;
            }
            if (stringIter != stringEnd && nextCharacterIsConflicting(*stringIter)) {
                return TokenRejectResult{"Invalid character '" + std::string(1, *stringIter) + "' in identifier", stringIter};
            }
            Token token = TokenFactory::getIdentifierToken(value, codeBegin, stringStart);
            return TokenAcceptResult{token, stringIter};
        }
};

export class NumberAcceptor : public TokenAcceptor {
    public:
        NumberAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            if (stringStart == stringEnd || !std::isdigit(*stringStart)) {
                return TokenRejectResult{"Not a number", stringStart};
            }
            enum State {
                INT,
                DOT,
                FLOAT,
            };
            std::string value;
            State state = INT;
            while (stringIter != stringEnd) {
                if (state == INT) {
                    if (*stringIter == '.') {
                        state = DOT;
                    }
                    else if (!std::isdigit(*stringIter)) {
                        break;
                    }
                }
                else if (state == DOT) {
                    if (!std::isdigit(*stringIter)) {
                        break;
                    }
                    state = FLOAT;
                }
                else if (state == FLOAT) {
                    if (!std::isdigit(*stringIter)) {
                        break;
                    }
                }
                // Accept the character
                value += *stringIter;
                stringIter++;
            }
            if (state == DOT) {
                return TokenRejectResult{"Invalid digit '.' in numeric constant", stringIter};
            }
            if (stringIter != stringEnd && nextCharacterIsConflicting(*stringIter)) {
                return TokenRejectResult{"Invalid digit '" + std::string(1, *stringIter) + "' in numeric constant", stringIter};
            }
            Token token = state == FLOAT ? TokenFactory::getFloatLiteralToken(value, codeBegin, stringStart) : TokenFactory::getIntegerLiteralToken(value, codeBegin, stringStart);
            return TokenAcceptResult{token, stringIter};
        }
};

export class StringAcceptor : public TokenAcceptor {
    public:
        StringAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            if (stringStart == stringEnd || *stringStart != '"') {
                return TokenRejectResult{"Not a string", stringStart};
            }
            enum State {
                NORMAL,
                ESCAPE,
            };
            std::string value;
            value += *stringIter;
            stringIter++;
            State state = NORMAL;
            while (stringIter != stringEnd && !(state == NORMAL && *stringIter == '"')) {
                if (*stringIter == '\n') {
                    return TokenRejectResult{"Unexpected newline in string constant", stringIter};
                }
                value += *stringIter;
                // Decide whether the *next* character is an escape character
                if (state == NORMAL) {
                    if (*stringIter == '\\') {
                        state = ESCAPE;
                    }
                } else if (state == ESCAPE) {
                    state = NORMAL;
                }
                stringIter++;
            }
            if (stringIter == stringEnd) {
                return TokenRejectResult{"Expected a double quote", stringIter};
            }
            value += *stringIter;
            stringIter++;
            if (stringIter != stringEnd && nextCharacterIsConflicting(*stringIter)) {
                return TokenRejectResult{"Invalid character '" + std::string(1, *stringIter) + "' in string constant", stringIter};
            }
            Token token = TokenFactory::getStringLiteralToken(value, codeBegin, stringStart);
            return TokenAcceptResult{token, stringIter};
        }
};

export class KeywordAcceptor : public TokenAcceptor {
    public:
        KeywordAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            if (stringStart == stringEnd || !std::isalpha(*stringStart) || *stringStart == '_') {
                return TokenRejectResult{"Not a keyword", stringStart};
            }
            const int maxLength = TokenFactory::longestKeywordLength;
            std::string value;
            while (stringIter != stringEnd && (std::isalpha(*stringIter) || *stringIter == '_') && value.size() < maxLength) {
                value += *stringIter;
                stringIter++;
            }
            const std::optional<Token> token = TokenFactory::findKeywordToken(value, codeBegin, stringStart);
            if (!token.has_value() || (stringIter != stringEnd && nextCharacterIsConflicting(*stringIter))) {
                return TokenRejectResult{"Not a keyword: " + value, stringStart};
            }
            return TokenAcceptResult{token.value(), stringIter};
        }
};

export class OperatorAcceptor : public TokenAcceptor {
    public:
        OperatorAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            if (stringStart == stringEnd || !std::ispunct(*stringStart)) {
                return TokenRejectResult{"Expected a operator character", stringStart};
            }
            const int maxLength = TokenFactory::longestOperatorLength;
            std::string value;
            std::optional<TokenAcceptResult> currentBestResult;
            while (stringIter != stringEnd && value.size() < maxLength) {
                value += *stringIter;
                const std::optional<Token> token = TokenFactory::findOperatorToken(value, codeBegin, stringStart);
                if (token.has_value()) {
                    currentBestResult.emplace(token.value(), stringIter + 1);
                }
                stringIter++;
            }
            if (!currentBestResult.has_value()) {
                return TokenRejectResult{"Not an operator: " + value, stringStart};
            }
            return currentBestResult.value();
        }
};

export class PunctuatorAcceptor : public TokenAcceptor {
    public:
        PunctuatorAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, const std::string_view::const_iterator stringEnd, const std::string_view::const_iterator codeBegin) override {
            const auto stringStart = stringIter;
            const int maxLength = TokenFactory::longestPunctuatorLength;
            std::string value;
            if (stringStart == stringEnd || !std::ispunct(*stringStart)) {
                return TokenRejectResult{"Expected a punctuator character", stringStart};
            }
            std::optional<TokenAcceptResult> currentBestResult;
            while (stringIter != stringEnd && value.size() < maxLength) {
                value += *stringIter;
                const std::optional<Token> token = TokenFactory::findPunctuatorToken(value, codeBegin, stringStart);
                if (token.has_value()) {
                    currentBestResult.emplace(token.value(), stringIter + 1);
                }
                stringIter++;
            }
            if (!currentBestResult.has_value()) {
                return TokenRejectResult{"Not a punctuator: " + value, stringStart};
            }
            return currentBestResult.value();
        }
};
