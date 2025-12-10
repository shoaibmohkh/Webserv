#include "../include/config_headers/Tokenizer.hpp"
#include <fstream>
#include <sstream>

int main() {
    std::ifstream file("webserv.conf");
    if (!file) {
        std::cerr << "Could not open config file.\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    Tokenizer t(buffer.str());
    std::vector<Token> tokens = t.tokenize();

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i].line << ": ";

        if (tokens[i].type == WORD)
            std::cout << "[WORD] " << tokens[i].value;
        else if (tokens[i].type == L_BRACE)
            std::cout << "[{]";
        else if (tokens[i].type == R_BRACE)
            std::cout << "[}]";
        else if (tokens[i].type == SEMICOLON)
            std::cout << "[;]";

        std::cout << "\n";
    }
}