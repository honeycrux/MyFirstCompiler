module;

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <memory>
#include <numeric>
#include <algorithm>

export module lexer;

import token;
import tokenacceptor;

export using LexicalError = std::string;

export class Lexer {
    private:
        const std::vector<std::unique_ptr<TokenAcceptor>> acceptors;

        static std::vector<std::unique_ptr<TokenAcceptor>> createAcceptors() {
            std::vector<std::unique_ptr<TokenAcceptor>> acceptors;

            // Initialize acceptors in our desired order
            acceptors.push_back(std::make_unique<NumberAcceptor>());
            acceptors.push_back(std::make_unique<StringAcceptor>());
            acceptors.push_back(std::make_unique<KeywordAcceptor>());
            acceptors.push_back(std::make_unique<IdentifierAcceptor>());
            acceptors.push_back(std::make_unique<OperatorAcceptor>());
            acceptors.push_back(std::make_unique<PunctuatorAcceptor>());

            return acceptors;
        }

        static std::string formatPosition(const std::string_view::const_iterator begin, const std::string_view::const_iterator where) {
            const int line = std::count(begin, where, '\n') + 1;
            std::string_view::const_iterator lineStart = std::find_if(std::reverse_iterator<std::string_view::const_iterator>(where), std::reverse_iterator<std::string_view::const_iterator>(begin), [](char c) { return c == '\n'; }).base();
            const int column = std::distance(lineStart, where) + 1;
            return std::to_string(line) + ":" + std::to_string(column);
        }

    public:
        Lexer(): acceptors(createAcceptors()) {}

        std::variant<std::vector<Token>, LexicalError> acceptCode(const std::string_view code) const {
            auto codeIter = code.begin();
            const auto codeEnd = code.end();
            std::vector<Token> tokens;
            while (codeIter != codeEnd) {
                // 1. Skip whitespace
                while (codeIter != codeEnd && std::isspace(*codeIter)) {
                    codeIter++;
                }
                // 2. Try each acceptor
                bool accepted = false;
                for (const auto& acceptor : acceptors) {
                    auto result = acceptor->accept(codeIter, codeEnd, code.begin());
                    if (std::holds_alternative<AcceptResult>(result)) {
                        AcceptResult acceptResult = std::get<AcceptResult>(result);
                        tokens.push_back(acceptResult.token);
                        codeIter = acceptResult.next;
                        accepted = true;
                        break;
                    }
                    else if (std::holds_alternative<RejectResult>(result)) {
                        RejectResult rejectResult = std::get<RejectResult>(result);
                        bool iterMoved = rejectResult.where != codeIter;
                        if (iterMoved) {
                            return LexicalError(rejectResult.message + " (at position " + formatPosition(code.begin(), rejectResult.where) + ")");
                        }
                    }
                }
                // 3. Error if no acceptor accepted
                if (codeIter != codeEnd && !accepted) {
                    return LexicalError("Unexpected token: " + std::string(1, *codeIter) + " (at position " + formatPosition(code.begin(), codeIter) + ")");
                }
            }
            return tokens;
        }

        std::string getPrintString(const std::vector<Token>& tokens) const {
            std::ostringstream oss;
            oss << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringPrint(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringPrint();
            });
            return oss.str();
        }

        std::string getWriteString(const std::vector<Token>& tokens) const {
            std::ostringstream oss;
            oss << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringWrite(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringWrite();
            });
            return oss.str();
        }
};
