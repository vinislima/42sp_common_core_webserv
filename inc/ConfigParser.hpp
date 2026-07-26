#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib> // Para o atoi()

#include "ServerConfig.hpp" // <-- Incluímos a nova classe

class ConfigParser {
private:
    std::string                 _configFile;
    std::vector<std::string>    _tokens;
    std::vector<ServerConfig>   _servers; // <-- Nossa lista de servidores prontos

    void    _tokenize(const std::string& content);
    
    // Novos métodos de arquitetura para a issue atual:
    void    _buildTree();
    void    _parseListen(ServerConfig& server, size_t& i);
    void    _parseServerName(ServerConfig& server, size_t& i);

public:
    // Forma Canônica Ortodoxa
    ConfigParser();
    ConfigParser(const std::string& filename);
    ConfigParser(const ConfigParser& src);
    ConfigParser& operator=(const ConfigParser& rhs);
    ~ConfigParser();

    void                        parse();
    std::vector<std::string>    getTokens() const;
    std::vector<ServerConfig>   getServers() const; // <-- Para pegar os servidores prontos
};

#endif