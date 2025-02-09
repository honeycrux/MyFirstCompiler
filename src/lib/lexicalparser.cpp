module;

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <variant>
#include <numeric>
#include <sstream>

export module lexicalparser;

import token;
import tokenacceptor;

export using LexicalError = std::string;

export class LexicalParser {
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
        LexicalParser(): acceptors(createAcceptors()) {}

        std::string readCodeFile(const std::string_view filenamesv) const {
            const std::string filename(filenamesv);
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            const std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return code;
        }

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
            std::ostringstream ss;
            ss << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringPrint(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringPrint();
            });
            return ss.str();
        }

        std::string getWriteString(const std::vector<Token>& tokens) const {
            std::ostringstream ss;
            ss << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringWrite(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringWrite();
            });
            return ss.str();
        }

        void printTokens(const std::vector<Token>& tokens) const {
            std::cout << getPrintString(tokens) << std::endl;
        }

        void writeTokensToFile(const std::vector<Token>& tokens, const std::string_view filenamesv) const {
            const std::string filename(filenamesv);
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            file << getWriteString(tokens) << std::endl;
        }

        int run(const std::string_view codeFile, const std::string_view tokenFile) const {
            const auto code = readCodeFile(codeFile);
            const auto result = acceptCode(code);
            if (std::holds_alternative<LexicalError>(result)) {
                std::cerr << std::get<LexicalError>(result) << std::endl;
                return 1;
            }
            const auto tokens = std::get<std::vector<Token>>(result);
            printTokens(tokens);
            writeTokensToFile(tokens, tokenFile);
            return 0;
        }
};
