#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <iostream>

class ServerConfig {
private:
    std::string                 _host;
    int                         _port;
    std::vector<std::string>    _serverNames;

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

    // Getters (Para a sua classe Server usar no futuro)
    std::string                 getHost() const;
    int                         getPort() const;
    std::vector<std::string>    getServerNames() const;
};

#endif