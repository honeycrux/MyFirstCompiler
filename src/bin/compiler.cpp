module;

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <iostream>
#include <fstream>
#include <memory>

export module compiler;

import token;
import lexer;
import ast;
import parser;

export class Compiler {
    private:
        Lexer lexer;
        Parser parser;

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
            for (const auto& token : tokens) {
                file << token;
            }
        }

    public:
        Compiler(): lexer(), parser() {}

        int run(const std::string_view codeFile, const std::string_view tokenFile) const {
            const auto code = readCodeFile(codeFile);
            const auto result = lexer.acceptCode(code);
            if (std::holds_alternative<LexerError>(result)) {
                std::cerr << std::get<LexerError>(result) << std::endl;
                return 1;
            }
            const auto& tokens = std::get<std::vector<Token>>(result);
            printTokens(tokens);
            writeTokensToFile(tokens, tokenFile);

            const auto parseResult = parser.parse(tokens);
            if (std::holds_alternative<ParserError>(parseResult)) {
                std::cerr << std::get<ParserError>(parseResult) << std::endl;
                return 1;
            }
            const auto& ast = std::get<std::unique_ptr<AstNode>>(parseResult);

            const auto typeCheckResult = ast->startTypeCheck();
            if (std::holds_alternative<TypeCheckError>(typeCheckResult)) {
                const auto typeCheckError = std::get<TypeCheckError>(typeCheckResult);
                std::cerr << typeCheckError.message + " (at position " + typeCheckError.where + ")" << std::endl;
                return 1;
            }

            std::cout << ast->toQuadrupleString() << std::endl;

            return 0;
        }
};

int main() {
    Compiler compiler;
    const int exitCode = compiler.run("code.txt", "tokens.txt");
    return exitCode;
}
