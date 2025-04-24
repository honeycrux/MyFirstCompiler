module;

#include <optional>
#include <string_view>

export module terminalfactory;

import tokenregistry;
import parserbase;

export namespace TerminalFactory {
    Terminal getIdentifier() {
        return Terminal{TokenRegistry::identifierId};
    }

    Terminal getIntegerLiteral() {
        return Terminal{TokenRegistry::integerLiteralId};
    }

    Terminal getRealLiteral() {
        return Terminal{TokenRegistry::realLiteralId};
    }

    Terminal getStringLiteral() {
        return Terminal{TokenRegistry::stringLiteralId};
    }

    std::optional<Terminal> getKeyword(std::string_view keyword) {
        auto it = TokenRegistry::keywordIdMap.find(keyword);
        if (it == TokenRegistry::keywordIdMap.end()) {
            return std::nullopt;
        }
        return Terminal{it->second};
    }

    std::optional<Terminal> getOperator(std::string_view op) {
        auto it = TokenRegistry::operatorIdMap.find(op);
        if (it == TokenRegistry::operatorIdMap.end()) {
            return std::nullopt;
        }
        return Terminal{it->second};
    }

    std::optional<Terminal> getPunctuator(std::string_view punctuator) {
        auto it = TokenRegistry::punctuatorIdMap.find(punctuator);
        if (it == TokenRegistry::punctuatorIdMap.end()) {
            return std::nullopt;
        }
        return Terminal{it->second};
    }
}
