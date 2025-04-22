#include <iostream>

import compiler;

int main() {
    Compiler compiler;
    const int exitCode = compiler.run("code.txt", "tokens.txt");
    return exitCode;
}
