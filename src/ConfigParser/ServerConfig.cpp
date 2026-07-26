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
		this->_errorPages = rhs._errorPages;
        this->_clientMaxBodySize = rhs._clientMaxBodySize;
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

void ServerConfig::addErrorPage(int code, const std::string& uri) {
    this->_errorPages[code] = uri;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
    this->_clientMaxBodySize = size;
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

std::map<int, std::string> ServerConfig::getErrorPages() const {
    return this->_errorPages;
}

size_t ServerConfig::getClientMaxBodySize() const {
    return this->_clientMaxBodySize;
}