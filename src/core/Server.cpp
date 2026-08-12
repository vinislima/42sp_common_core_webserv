#include "../../inc/Server.hpp"
#include "../../inc/Request.hpp"
#include "../../inc/Response.hpp" 

Server::Server() {}

Server::Server(const std::vector<ServerConfig>& configs) : _configs(configs) {}

Server::Server(const Server& src) {
    *this = src;
}

Server& Server::operator=(const Server& rhs) {
    if (this != &rhs) {
        this->_configs = rhs._configs;
        this->_listenSockets = rhs._listenSockets;
    }
    return *this;
}

Server::~Server() {
    for (size_t i = 0; i < _listenSockets.size(); ++i) {
        close(_listenSockets[i]);
    }
}

void Server::_setupSockets() {
    for (size_t i = 0; i < _configs.size(); ++i) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) throw std::runtime_error("Erro: Falha ao criar o socket.");
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) throw std::runtime_error("Erro: Falha no setsockopt.");
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) throw std::runtime_error("Erro: Falha ao definir socket como n o bloqueante.");
        if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) < 0) throw std::runtime_error("Erro: Falha ao definir socket com FD_CLOEXEC.");
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(_configs[i].getPort());
        addr.sin_addr.s_addr = inet_addr(_configs[i].getHost().c_str());
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) throw std::runtime_error("Erro: Falha no bind.");
        if (listen(sockfd, 128) < 0) throw std::runtime_error("Erro: Falha no listen.");
        _listenSockets.push_back(sockfd);
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollFds.push_back(pfd);
        std::cout << "[SUCESSO] Servidor ouvindo em " << _configs[i].getHost() << ":" << _configs[i].getPort() << "\n";
    }
}

bool Server::_isListenSocket(int fd) {
    for (size_t i = 0; i < _listenSockets.size(); ++i) {
        if (_listenSockets[i] == fd) return true;
    }
    return false;
}

void Server::_acceptNewConnection(int listenFd) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) return;
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    fcntl(clientFd, F_SETFD, FD_CLOEXEC);
    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN | POLLOUT; 
    pfd.revents = 0;
    _pollFds.push_back(pfd);
    std::cout << "[REDE] Novo cliente conectado! FD: " << clientFd << " IP: " << inet_ntoa(clientAddr.sin_addr) << "\n";
}

bool Server::_handleClientRead(int clientFd) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) return false;
    buffer[bytesRead] = '\0';
    Request& req = _clientRequests[clientFd];
    req.appendToRaw(std::string(buffer, bytesRead));
    try {
        req.parse();
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Parse pendente ou erro: " << e.what() << "\n";
        return true; 
    }
    return true;
}

bool Server::_handleClientWrite(int clientFd) {
    if (_clientRequests.find(clientFd) != _clientRequests.end()) {
        Request& req = _clientRequests[clientFd];
        
        if (req.isComplete()) {
            Response res;
            res.build(req); 
            
            std::string rawRes = res.getRawResponse();
            
            ssize_t bytesSent = send(clientFd, rawRes.c_str(), rawRes.length(), 0);
            if (bytesSent < 0) {
                std::cerr << "[ERRO] Falha ao enviar resposta ao cliente\n";
            } else {
                std::cout << "[HTTP] Resposta enviada ao FD " << clientFd << " (Tamanho: " << bytesSent << " bytes)\n";
            }
            
            return false; 
        }
    }
    return true; 
}

void Server::_runEventLoop() {
    std::cout << "\n[INFO] Servidor online. Aguardando conexões...\n";
    while (true) {
        int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
        if (ret < 0) throw std::runtime_error("Erro: Falha na função poll()");

        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents == 0) continue;

            if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::cout << "[REDE] Erro/HUP no cliente. FD: " << _pollFds[i].fd << "\n";
                close(_pollFds[i].fd);
                _clientRequests.erase(_pollFds[i].fd); 
                _pollFds.erase(_pollFds.begin() + i);
                i--;
                continue;
            }

            if (_pollFds[i].revents & POLLIN) {
                if (_isListenSocket(_pollFds[i].fd)) {
                    _acceptNewConnection(_pollFds[i].fd);
                } else {
                    bool keepAlive = _handleClientRead(_pollFds[i].fd);
                    if (!keepAlive) {
                        std::cout << "[REDE] Cliente desconectado na leitura. FD: " << _pollFds[i].fd << "\n";
                        close(_pollFds[i].fd);
                        _clientRequests.erase(_pollFds[i].fd);
                        _pollFds.erase(_pollFds.begin() + i);
                        i--;
                        continue;
                    }
                }
            }

            if (_pollFds[i].revents & POLLOUT) {
                if (!_isListenSocket(_pollFds[i].fd)) {
                    bool keepAlive = _handleClientWrite(_pollFds[i].fd);
                    
                    if (!keepAlive) {
                        std::cout << "[REDE] Fechando conexão (fim da requisição). FD: " << _pollFds[i].fd << "\n";
                        close(_pollFds[i].fd);
                        _clientRequests.erase(_pollFds[i].fd); 
                        _pollFds.erase(_pollFds.begin() + i);
                        i--;
                        continue;
                    }
                }
            }
        }
    }
}

void Server::start() {
    std::cout << "Inicializando servidor...\n";
    _setupSockets();
    _runEventLoop();
}