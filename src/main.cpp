#include "../inc/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv) {
    std::string configFile = (argc == 2) ? argv[1] : "default.conf";
    
    try {
        ConfigParser parser(configFile);
        parser.parse();
        
        // Pega a lista de servidores em vez dos tokens crus
        std::vector<ServerConfig> servers = parser.getServers();
        
        std::cout << "--- SERVIDORES CONFIGURADOS ---\n\n";
        
        for (size_t i = 0; i < servers.size(); ++i) {
            std::cout << "[Servidor " << i + 1 << "]\n";
            std::cout << "  -> Host:  " << servers[i].getHost() << "\n";
            std::cout << "  -> Porta: " << servers[i].getPort() << "\n";
            
            std::cout << "  -> Nomes: ";
            std::vector<std::string> names = servers[i].getServerNames();
            for (size_t j = 0; j < names.size(); ++j) {
                std::cout << names[j] << " ";
            }
            std::cout << "\n\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}