#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <poll.h>
#include <cerrno>
#include <map>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

#include "ServerConfig.hpp"
#include "Request.hpp"

class Server {
private:
    std::vector<ServerConfig>   _configs;
    std::vector<int>            _listenSockets;
    std::vector<struct pollfd>  _pollFds;
    std::map<int, Request>      _clientRequests;

    void    _setupSockets();
    bool    _isListenSocket(int fd);
    void    _acceptNewConnection(int listenFd);
    
    bool    _handleClientRead(int clientFd);
    
    bool    _handleClientWrite(int clientFd); 
    
    void    _runEventLoop();

public:
    Server();
    Server(const std::vector<ServerConfig>& configs);
    Server(const Server& src);
    Server& operator=(const Server& rhs);
    ~Server();

    void    start();
};

#endif