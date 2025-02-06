module;

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <variant>
#include <numeric>

export module lexicalparser;

import token;
import tokenacceptor;

using LexicalError = std::string;

export class LexicalParser {
    private:
        std::vector<std::unique_ptr<TokenAcceptor>> acceptors;

        void initializeAcceptors() {
            // Initialize acceptors in our desired order
            if (!acceptors.empty()) {
                throw std::runtime_error("Acceptors already initialized");
            }
            acceptors.push_back(std::make_unique<NumberAcceptor>());
            acceptors.push_back(std::make_unique<StringAcceptor>());
            acceptors.push_back(std::make_unique<KeywordAcceptor>());
            acceptors.push_back(std::make_unique<IdentifierAcceptor>());
            acceptors.push_back(std::make_unique<OperatorAcceptor>());
            acceptors.push_back(std::make_unique<PunctuatorAcceptor>());
        }

        std::string readCodeFile(const std::string_view filenamesv) {
            std::string filename(filenamesv);
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return code;
        }

        std::string formatPosition(std::string_view::const_iterator iter, std::string_view::const_iterator begin) {
            return " (at position " + std::to_string(std::distance(begin, iter) + 1) + ")";
        }

        std::variant<std::vector<Token>, LexicalError> acceptCode(const std::string_view code) {
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
                if (!accepted) {
                    return LexicalError("Unexpected token: " + std::string(1, *codeIter) + formatPosition(codeIter, code.begin()));
                }
            }
            return tokens;
        }

        void printTokens(const std::vector<Token>& tokens) {
            std::cout << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringPrint(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringPrint();
            }) << std::endl;
        }

        void writeTokensToFile(const std::vector<Token>& tokens, const std::string_view filenamesv) {
            std::string filename(filenamesv);
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            file << std::accumulate(tokens.begin() + 1, tokens.end(), tokens[0].toStringWrite(), [](const std::string& acc, const Token& token) {
                return acc + ", " + token.toStringWrite();
            }) << std::endl;
        }

    public:
        LexicalParser() {
            initializeAcceptors();
        }

        int run(const std::string_view codeFile, const std::string_view tokenFile) {
            auto code = readCodeFile(codeFile);
            auto result = acceptCode(code);
            if (std::holds_alternative<LexicalError>(result)) {
                std::cerr << std::get<LexicalError>(result) << std::endl;
                return 1;
            }
            auto tokens = std::get<std::vector<Token>>(result);
            printTokens(tokens);
            writeTokensToFile(tokens, tokenFile);
            return 0;
        }
};
