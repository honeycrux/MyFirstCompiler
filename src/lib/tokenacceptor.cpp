module;

#include <string>
#include <memory>
#include <cctype>
#include <optional>
#include <variant>

export module tokenacceptor;

import token;
import tokenfactory;

export struct AcceptResult {
    Token token;
    std::string_view::const_iterator next;
};

export struct RejectResult {
    std::string message;
    std::string_view::const_iterator where;
};

using TokenAcceptorResult = std::variant<AcceptResult, RejectResult>;

export class TokenAcceptor {
    public:
        TokenAcceptor() {}
        virtual ~TokenAcceptor() {}
        virtual TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) = 0;
};

export class IdentifierAcceptor : public TokenAcceptor {
    public:
        IdentifierAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            std::string value;
            if (stringIter == stringEnd || !(std::isalpha(*stringIter) || *stringIter == '_')) {
                return RejectResult{"Expected an alphabetic character or underscore", stringIter};
            }
            while (stringIter != stringEnd && (std::isalnum(*stringIter) || *stringIter == '_')) {
                value += *stringIter;
                stringIter++;
            }
            std::optional<Token> token = TokenFactory::getIdentifierToken(value);
            if (!token.has_value()) {
                throw std::runtime_error("Unexpected: TokenFactory invalidated identifier (" + value + ")");
            }
            return AcceptResult{token.value(), stringIter};
        }
};

export class NumberAcceptor : public TokenAcceptor {
    public:
        NumberAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            enum State {
                INT,
                DOT,
                REAL,
            };
            std::string value;
            if (stringIter == stringEnd || !std::isdigit(*stringIter)) {
                return RejectResult{"Expected a digit", stringIter};
            }
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
                        return RejectResult{"Expected a digit after the dot", stringIter};
                    }
                    state = REAL;
                }
                else if (state == REAL) {
                    if (!std::isdigit(*stringIter)) {
                        break;
                    }
                }
                // Accept the character
                value += *stringIter;
                stringIter++;
            }
            if (state == DOT) {
                return RejectResult{"Expected a digit after the dot", stringIter};
            }
            std::optional<Token> token = TokenFactory::getNumberToken(value);
            if (!token.has_value()) {
                throw std::runtime_error("Unexpected: TokenFactory invalidated number (" + value + ")");
            }
            return AcceptResult{token.value(), stringIter};
        }
};

export class StringAcceptor : public TokenAcceptor {
    public:
        StringAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            std::string value;
            if (stringIter == stringEnd || *stringIter != '"') {
                return RejectResult{"Expected a double quote", stringIter};
            }
            value += *stringIter;
            stringIter++;
            enum State {
                NORMAL,
                ESCAPE,
            };
            State state = NORMAL;
            while (stringIter != stringEnd && !(state == NORMAL && *stringIter == '"')) {
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
                return RejectResult{"Expected a double quote", stringIter};
            }
            value += *stringIter;
            stringIter++;
            std::optional<Token> token = TokenFactory::getStringToken(value);
            if (!token.has_value()) {
                throw std::runtime_error("Unexpected: TokenFactory invalidated string (" + value + ")");
            }
            return AcceptResult{token.value(), stringIter};
        }
};

export class KeywordAcceptor : public TokenAcceptor {
    public:
        KeywordAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            auto originalIter = stringIter;
            int maxLength = TokenFactory::longestKeywordLength;
            std::string value;
            if (stringIter == stringEnd || !std::isalpha(*stringIter) || *stringIter == '_') {
                return RejectResult{"Expected an alphabetic character", originalIter};
            }
            while (stringIter != stringEnd && (std::isalpha(*stringIter) || *stringIter == '_') && value.size() < maxLength) {
                value += *stringIter;
                stringIter++;
            }
            std::optional<Token> token = TokenFactory::getKeywordToken(value);
            if (!token.has_value()) {
                return RejectResult{"Not a keyword: " + value, originalIter};
            }
            return AcceptResult{token.value(), stringIter};
        }
};

export class OperatorAcceptor : public TokenAcceptor {
    public:
        OperatorAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            auto originalIter = stringIter;
            int maxLength = TokenFactory::longestOperatorLength;
            std::string value;
            if (stringIter == stringEnd || !std::ispunct(*stringIter)) {
                return RejectResult{"Expected a operator character", originalIter};
            }
            std::optional<AcceptResult> currentBestResult;
            while (stringIter != stringEnd && value.size() < maxLength) {
                value += *stringIter;
                std::optional<Token> token = TokenFactory::getOperatorToken(value);
                if (token.has_value()) {
                    currentBestResult.emplace(token.value(), stringIter + 1);
                }
                stringIter++;
            }
            if (!currentBestResult.has_value()) {
                return RejectResult{"Not an operator: " + value, originalIter};
            }
            return currentBestResult.value();
        }
};

export class PunctuatorAcceptor : public TokenAcceptor {
    public:
        PunctuatorAcceptor() {}
        TokenAcceptorResult accept(std::string_view::const_iterator stringIter, std::string_view::const_iterator stringEnd) override {
            auto originalIter = stringIter;
            int maxLength = TokenFactory::longestPunctuatorLength;
            std::string value;
            if (stringIter == stringEnd || !std::ispunct(*stringIter)) {
                return RejectResult{"Expected a punctuator character", originalIter};
            }
            std::optional<AcceptResult> currentBestResult;
            while (stringIter != stringEnd && value.size() < maxLength) {
                value += *stringIter;
                std::optional<Token> token = TokenFactory::getPunctuatorToken(value);
                if (token.has_value()) {
                    currentBestResult.emplace(token.value(), stringIter + 1);
                }
                stringIter++;
            }
            if (!currentBestResult.has_value()) {
                return RejectResult{"Not a punctuator: " + value, originalIter};
            }
            return currentBestResult.value();
        }
};
