module;

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <memory>
#include <numeric>

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

        static std::string formatPosition(const std::string_view::const_iterator iter, const std::string_view::const_iterator begin) {
            return " (at position " + std::to_string(std::distance(begin, iter) + 1) + ")";
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
                    auto result = acceptor->accept(codeIter, codeEnd);
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
                            return LexicalError(rejectResult.message + formatPosition(rejectResult.where, code.begin()));
                        }
                    }
                }
                // 3. Error if no acceptor accepted
                if (codeIter != codeEnd && !accepted) {
                    return LexicalError("Unexpected token: " + std::string(1, *codeIter) + formatPosition(codeIter, code.begin()));
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
