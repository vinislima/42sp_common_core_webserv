#include "../inc/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv) {
    std::string configFile = (argc == 2) ? argv[1] : "default.conf";
    
    try {
        ConfigParser parser(configFile);
        parser.parse();
        
        std::vector<std::string> tokens = parser.getTokens();
        std::cout << "--- TOKENS ENCONTRADOS ---\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << "[" << tokens[i] << "]\n";
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
