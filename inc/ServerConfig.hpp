#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>

class ServerConfig {
private:
    std::string                 _host;
    int                         _port;
    std::vector<std::string>    _serverNames;
    std::map<int, std::string>  _errorPages;
    size_t                      _clientMaxBodySize;

public:
    // Forma Canônica Ortodoxa
    ServerConfig();
    ServerConfig(const ServerConfig& src);
    ServerConfig& operator=(const ServerConfig& rhs);
    ~ServerConfig();

    // Setters (Para o ConfigParser usar)
    void    setHost(const std::string& host);
    void    setPort(int port);
    void    addServerName(const std::string& name);
    void    addErrorPage(int code, const std::string& uri);
    void    setClientMaxBodySize(size_t size);

    // Getters (Para a sua classe Server usar no futuro)
    std::string                 getHost() const;
    int                         getPort() const;
    std::vector<std::string>    getServerNames() const;
    std::map<int, std::string>  getErrorPages() const;
    size_t                      getClientMaxBodySize() const;
};

#endif