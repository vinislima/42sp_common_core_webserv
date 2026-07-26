#include "../../inc/ServerConfig.hpp"

// =============================================================================
// Construtores e Destrutor (Forma Canônica)
// =============================================================================

ServerConfig::ServerConfig() : _host("0.0.0.0"), _port(80) {}

ServerConfig::ServerConfig(const ServerConfig& src) {
    *this = src;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& rhs) {
    if (this != &rhs) {
        this->_host = rhs._host;
        this->_port = rhs._port;
        this->_serverNames = rhs._serverNames;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

// =============================================================================
// Setters
// =============================================================================

void ServerConfig::setHost(const std::string& host) {
    this->_host = host;
}

void ServerConfig::setPort(int port) {
    this->_port = port;
}

void ServerConfig::addServerName(const std::string& name) {
    this->_serverNames.push_back(name);
}

// =============================================================================
// Getters
// =============================================================================

std::string ServerConfig::getHost() const {
    return this->_host;
}

int ServerConfig::getPort() const {
    return this->_port;
}

std::vector<std::string> ServerConfig::getServerNames() const {
    return this->_serverNames;
}