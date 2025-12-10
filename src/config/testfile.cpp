// #include "../../include/config_headers/Tokenizer.hpp"
// #include <fstream>
// #include <sstream>

// int main() {
//     std::ifstream file("webserv.conf");
//     if (!file) {
//         std::cerr << "Could not open config file.\n";
//         return 1;
//     }

//     std::stringstream buffer;
//     buffer << file.rdbuf();

//     Tokenizer t(buffer.str());
//     std::vector<Token> tokens = t.tokenize();

//     for (size_t i = 0; i < tokens.size(); ++i) {
//         std::cout << tokens[i].line << ": ";

//         if (tokens[i].type == WORD)
//             std::cout << "[WORD] " << tokens[i].value;
//         else if (tokens[i].type == L_BRACE)
//             std::cout << "[{]";
//         else if (tokens[i].type == R_BRACE)
//             std::cout << "[}]";
//         else if (tokens[i].type == SEMICOLON)
//             std::cout << "[;]";

//         std::cout << "\n";
//     }
// }

#include "../../include/config_headers/Tokenizer.hpp"
#include "../../include/config_headers/parser.hpp"
#include <fstream>
#include <sstream>

void printLocation(const LocationConfig &loc)
{
    std::cout << "    Location path: " << loc.path << "\n";
    std::cout << "      autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
    std::cout << "      upload_enable: " << (loc.uploadEnable ? "on" : "off") << "\n";
    std::cout << "      upload_store: " << loc.uploadStore << "\n";

    std::cout << "      allow_methods: ";
    for (size_t i = 0; i < loc.allowMethods.size(); ++i)
        std::cout << loc.allowMethods[i] << " ";
    std::cout << "\n";

    std::cout << "      return: " << loc.returnCode << " " << loc.returnPath << "\n";

    std::cout << "      cgi_extensions:\n";
    for (std::map<std::string, std::string>::const_iterator it = loc.cgiExtensions.begin();
         it != loc.cgiExtensions.end(); ++it)
    {
        std::cout << "        " << it->first << " -> " << it->second << "\n";
    }
}

void printServer(const ServerConfig &s)
{
    std::cout << "Server:\n";
    std::cout << "  listen: " << s.port << "\n";
    std::cout << "  client_max_body_size: " << s.client_Max_Body_Size << "\n";
    std::cout << "  root: " << s.root << "\n";
    std::cout << "  index: " << s.index << "\n";

    std::cout << "  error_pages:\n";
    for (std::map<int, std::string>::const_iterator it = s.error_Pages.begin();
         it != s.error_Pages.end(); ++it)
    {
        std::cout << "    " << it->first << " -> " << it->second << "\n";
    }

    std::cout << "  locations:\n";
    for (size_t i = 0; i < s.locations.size(); ++i)
        printLocation(s.locations[i]);
}

int main()
{
    std::ifstream file("webserv.conf");
    if (!file)
    {
        std::cerr << "Could not open config file.\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // Tokenize
    Tokenizer tokenizer(buffer.str());
    std::vector<Token> tokens = tokenizer.tokenize();

    // Parse
    Parser parser(tokens);
    Config conf = parser.parse();

    // Print result
    for (size_t i = 0; i < conf.servers.size(); ++i)
    {
        std::cout << "\n===========================\n";
        std::cout << " SERVER #" << i + 1 << "\n";
        std::cout << "===========================\n";
        printServer(conf.servers[i]);
    }

    return 0;
}
