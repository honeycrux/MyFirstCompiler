module;

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <iostream>
#include <fstream>

export module compiler;

import lexer;
import token;

export class Compiler {
    private:
        Lexer lexer;

        std::string readCodeFile(const std::string_view filenamesv) const {
            const std::string filename(filenamesv);
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            const std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return code;
        }

        void printTokens(const std::vector<Token>& tokens) const {
            std::cout << lexer.getPrintString(tokens) << std::endl;
        }

        void writeTokensToFile(const std::vector<Token>& tokens, const std::string_view filenamesv) const {
            const std::string filename(filenamesv);
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            file << lexer.getWriteString(tokens) << std::endl;
        }

    public:
        Compiler(): lexer() {}

        int run(const std::string_view codeFile, const std::string_view tokenFile) const {
            const auto code = readCodeFile(codeFile);
            const auto result = lexer.acceptCode(code);
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
