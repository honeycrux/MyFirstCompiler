#include <iostream>

import lexicalparser;

int main() {
    LexicalParser lexicalParser;
    const int exitCode = lexicalParser.run("code.txt", "tokens.txt");
    return exitCode;
}
