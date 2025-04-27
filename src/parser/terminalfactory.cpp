module;

#include <optional>
#include <string_view>
#include <stdexcept>

export module terminalfactory;

import token;
import tokenregistry;
import symbol;

export namespace TerminalFactory {
    Terminal getIdentifier() {
        return Terminal{TokenRegistry::identifierId, "identifier"};
    }

    Terminal getIntegerLiteral() {
        return Terminal{TokenRegistry::integerLiteralId, "integerLiteral"};
    }

    Terminal getFloatLiteral() {
        return Terminal{TokenRegistry::floatLiteralId, "floatLiteral"};
    }

    Terminal getStringLiteral() {
        return Terminal{TokenRegistry::stringLiteralId, "stringLiteral"};
    }

    Terminal getKeyword(std::string_view keyword) {
        auto it = TokenRegistry::keywordIdMap.find(keyword);
        if (it == TokenRegistry::keywordIdMap.end()) {
            throw std::runtime_error("Keyword not found: " + std::string{keyword});
        }
        return Terminal{it->second, keyword};
    }

    Terminal getOperator(std::string_view op) {
        auto it = TokenRegistry::operatorIdMap.find(op);
        if (it == TokenRegistry::operatorIdMap.end()) {
            throw std::runtime_error("Operator not found: " + std::string{op});
        }
        return Terminal{it->second, op};
    }

    Terminal getPunctuator(std::string_view punctuator) {
        auto it = TokenRegistry::punctuatorIdMap.find(punctuator);
        if (it == TokenRegistry::punctuatorIdMap.end()) {
            throw std::runtime_error("Punctuator not found: " + std::string{punctuator});
        }
        return Terminal{it->second, punctuator};
    }

    Terminal fromToken(const Token& token) {
        const TokenType type = token.getType();
        switch (type) {
            case TokenType::IDENTIFIER:
                return getIdentifier();
            case TokenType::INTEGER:
                return getIntegerLiteral();
            case TokenType::FLOAT:
                return getFloatLiteral();
            case TokenType::STRING:
                return getStringLiteral();
            case TokenType::KEYWORD: {
                return Terminal{token.getId(), token.getValue()};
            }
            case TokenType::OPERATOR: {
                return Terminal{token.getId(), token.getValue()};
            }
            case TokenType::PUNCTUATOR: {
                return Terminal{token.getId(), token.getValue()};
            }
        }
        throw std::runtime_error("Unknown token type");
    }
}
