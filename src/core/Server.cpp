/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:41 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:42 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/Server.hpp"
#include "../../inc/Request.hpp"
#include "../../inc/Response.hpp" 

#include <sys/wait.h>
#include <cstdlib>
#include <csignal>

Server::Server() {}

Server::Server(const std::vector<ServerConfig>& configs) : _configs(configs) {}

Server::Server(const Server& src) {
    *this = src;
}

Server& Server::operator=(const Server& rhs) {
    if (this != &rhs) {
        this->_configs = rhs._configs;
        this->_listenSockets = rhs._listenSockets;
        this->_listenFdToPort = rhs._listenFdToPort;
        this->_clientToListenFd = rhs._clientToListenFd;
    }
    return *this;
}

Server::~Server() {
    for (size_t i = 0; i < _listenSockets.size(); ++i) {
        close(_listenSockets[i]);
    }
}

void Server::_setupSockets() {
    std::vector<std::string> boundAddresses;
    
    for (size_t i = 0; i < _configs.size(); ++i) {
        std::ostringstream ss;
        ss << _configs[i].getHost() << ":" << _configs[i].getPort();
        std::string currentAddr = ss.str();

        bool alreadyBound = false;
        for (size_t j = 0; j < boundAddresses.size(); ++j) {
            if (boundAddresses[j] == currentAddr) {
                alreadyBound = true;
                break;
            }
        }
        if (alreadyBound) {
            std::cout << "[INFO] Socket para " << currentAddr << " ja est  aberto. Configurado como Virtual Host.\n";
            continue;
        }

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) throw std::runtime_error("Erro: Falha ao criar o socket.");

        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("Erro: Falha no setsockopt.");

        if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
            throw std::runtime_error("Erro: Falha ao definir socket como nao bloqueante.");

        if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) < 0)
            throw std::runtime_error("Erro: Falha ao definir socket com FD_CLOEXEC.");

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(_configs[i].getPort());
        addr.sin_addr.s_addr = inet_addr(_configs[i].getHost().c_str());

        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("Erro: Falha no bind no endere o " + currentAddr);

        if (listen(sockfd, 128) < 0)
            throw std::runtime_error("Erro: Falha no listen.");

        _listenSockets.push_back(sockfd);
        _listenFdToPort[sockfd] = _configs[i].getPort();

        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollFds.push_back(pfd);

        boundAddresses.push_back(currentAddr);
        std::cout << "[SUCESSO] Servidor ouvindo em " << currentAddr << "\n";
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
    pfd.events = POLLIN; 
    pfd.revents = 0;
    _pollFds.push_back(pfd);

    _clients[clientFd] = Client();
    _clientToListenFd[clientFd] = listenFd;

    std::cout << "[REDE] Novo cliente conectado! FD: " << clientFd << " IP: " << inet_ntoa(clientAddr.sin_addr) << "\n";
}

void Server::_closeClient(int clientFd, size_t pollIdx) {
    close(clientFd);
    _clients.erase(clientFd);
    _clientToListenFd.erase(clientFd);
    _pollFds.erase(_pollFds.begin() + pollIdx);
}

void Server::_erasePollFd(int fd) {
    for (size_t i = 0; i < _pollFds.size(); ++i) {
        if (_pollFds[i].fd == fd) {
            _pollFds.erase(_pollFds.begin() + i);
            return;
        }
    }
}

const ServerConfig* Server::_matchConfig(int clientFd, const std::string& hostHeader) const {
    const ServerConfig* defaultConfig = _configs.empty() ? NULL : &_configs[0];

    std::map<int, int>::const_iterator listenIt = _clientToListenFd.find(clientFd);
    if (listenIt == _clientToListenFd.end()) return defaultConfig;

    std::map<int, int>::const_iterator portIt = _listenFdToPort.find(listenIt->second);
    if (portIt == _listenFdToPort.end()) return defaultConfig;
    int clientPort = portIt->second;

    const ServerConfig* firstOnPort = NULL;
    for (size_t i = 0; i < _configs.size(); ++i) {
        if (_configs[i].getPort() != clientPort) continue;
        if (!firstOnPort) firstOnPort = &_configs[i];

        std::vector<std::string> names = _configs[i].getServerNames();
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == hostHeader) return &_configs[i];
        }
    }

    // No server_name matched: fall back to the first server{} listening on
    // that port (default "default server" per-port behavior, same as nginx).
    return firstOnPort ? firstOnPort : defaultConfig;
}

bool Server::_handleClientRead(int clientFd) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) return false;

    buffer[bytesRead] = '\0';
    Client& client = _clients[clientFd];
    client.req.appendToRaw(std::string(buffer, bytesRead));
    client.updateActivity();

    _parseClientRequest(clientFd);

    return true;
}

// Extracted from _handleClientRead so it can be reused by the Keep-Alive
// reset: when a 2nd request already arrived pipelined (together with the
// 1st, in the same recv()), its bytes are already in the buffer without a
// new recv() happening — so this parsing needs to be callable again
// without depending on another read event on the socket.
void Server::_parseClientRequest(int clientFd) {
    Client& client = _clients[clientFd];

    // Defensive limit: only client_max_body_size caps the body, nothing caps
    // the request-line/headers. A client that never sends the closing
    // "\r\n\r\n" (or sends one absurdly large header block) would make
    // _rawRequest grow forever in memory. Checked BEFORE parsing, so an
    // oversized block gets rejected even if it does contain a complete
    // "\r\n\r\n" — headers that large are already unreasonable.
    const size_t MAX_HEADER_BYTES = 8192;
    if (!client.req.areHeadersParsed() && client.req.getRawLength() > MAX_HEADER_BYTES) {
        std::cerr << "[ERRO] Request-line/headers excederam " << MAX_HEADER_BYTES << " bytes. FD: " << clientFd << "\n";
        client.req.setErrorCode(431);
        return;
    }

    try {
        if (!client.req.areHeadersParsed()) {
            client.req.parseHeadersOnly();

            if (client.req.areHeadersParsed() && !client.req.isBodyAuthorized()) {
                std::string hostHeader = client.req.getHeader("Host");
                size_t colonPos = hostHeader.find(':');
                if(colonPos != std::string::npos) {
                    hostHeader = hostHeader.substr(0, colonPos);
                }

                const ServerConfig* matchedConfig = _matchConfig(clientFd, hostHeader);

                size_t maxBodySize = matchedConfig->getClientMaxBodySize();

                // A location can override client_max_body_size for its own
                // route. The body hasn't arrived yet at this point, but the
                // URI (and query string) already have — strip the query
                // string the same way Response::build() does before
                // matching, so both agree on which location applies.
                std::string uriForMatch = client.req.getUri();
                size_t queryPos = uriForMatch.find('?');
                if (queryPos != std::string::npos) {
                    uriForMatch = uriForMatch.substr(0, queryPos);
                }
                const LocationConfig* loc = matchedConfig->getBestMatchLocation(uriForMatch);
                if (loc && loc->hasClientMaxBodySize()) {
                    maxBodySize = loc->getClientMaxBodySize();
                }

                client.req.setMaxBodySize(maxBodySize);

                std::string cl = client.req.getHeader("Content-Length");
                if (!cl.empty()) {
                    size_t contentLen = std::strtoul(cl.c_str(), NULL, 10);
                    if (contentLen > maxBodySize && maxBodySize > 0) {
                        client.req.setErrorCode(413);
                    }
                }
                client.req.setBodyAuthorized(true);
            }
        }

        if (client.req.areHeadersParsed() && client.req.getErrorCode() == 0) {
            client.req.parseBodyOnly();
        }

    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        if (errorMsg == "Body incompleto" || errorMsg == "Chunk incompleto") {
            return;
        }
        std::cerr << "[ERRO] Requisicao malformada ou invalida: " << errorMsg << "\n";
        client.req.setErrorCode(400);
    }
}

bool Server::_handleClientWrite(int clientFd) {
    if (_clients.find(clientFd) != _clients.end()) {
        Client& client = _clients[clientFd];
        
        if (client.req.isComplete()) {
            if (!client.isReadyToSend) {
                Response res;
                
                std::string hostHeader = client.req.getHeader("Host");
                size_t colonPos = hostHeader.find(':');
                if (colonPos != std::string::npos) {
                    hostHeader = hostHeader.substr(0, colonPos);
                }
                
                const ServerConfig* matchedConfig = _matchConfig(clientFd, hostHeader);

                res.build(client.req, *matchedConfig);
                
                // ==============================================================
                // CGI INTERCEPTOR
                // ==============================================================
                if (res.getCgiPid() != -1) {
                    client.isCgi = true;
                    client.cgiPid = res.getCgiPid();
                    client.cgiReadFd = res.getCgiReadFd();
                    client.cgiWriteFd = res.getCgiWriteFd();
                    client.cgiOutput = "";
                    client.cgiBytesWritten = 0;
                    client.cgiStart = time(NULL);

                    struct pollfd pfdRead;
                    pfdRead.fd = client.cgiReadFd;
                    pfdRead.events = POLLIN;
                    pfdRead.revents = 0;
                    _pollFds.push_back(pfdRead);
                    _cgiToClient[client.cgiReadFd] = clientFd;

                    if (!client.req.getBody().empty()) {
                        struct pollfd pfdWrite;
                        pfdWrite.fd = client.cgiWriteFd;
                        pfdWrite.events = POLLOUT;
                        pfdWrite.revents = 0;
                        _pollFds.push_back(pfdWrite);
                        _cgiToClient[client.cgiWriteFd] = clientFd;
                    } else {
                        close(client.cgiWriteFd);
                        client.cgiWriteFd = -1;
                    }

                    for (size_t k = 0; k < _pollFds.size(); ++k) {
                        if (_pollFds[k].fd == clientFd) {
                            _pollFds[k].events = POLLIN;
                            break;
                        }
                    }
                    return true;
                }
                // ==============================================================
                // FILE INTERCEPTOR (ASYNCHRONOUS DISK I/O)
                // ==============================================================
                else if (res.getFileReadFd() != -1 || res.getFileWriteFd() != -1) {
                    client.isFile = true;
                    client.fileReadFd = res.getFileReadFd();
                    client.fileWriteFd = res.getFileWriteFd();
                    client.fileBytesWritten = 0;
                    
                    // Puts the headers in the buffer, the file body will arrive via poll()
                    client.responseBuffer = res.getRawResponse(); 

                    if (client.fileReadFd != -1) {
                        struct pollfd pfdRead;
                        pfdRead.fd = client.fileReadFd;
                        pfdRead.events = POLLIN;
                        pfdRead.revents = 0;
                        _pollFds.push_back(pfdRead);
                        _fileToClient[client.fileReadFd] = clientFd;
                    } 
                    else if (client.fileWriteFd != -1) {
                        struct pollfd pfdWrite;
                        pfdWrite.fd = client.fileWriteFd;
                        pfdWrite.events = POLLOUT;
                        pfdWrite.revents = 0;
                        _pollFds.push_back(pfdWrite);
                        _fileToClient[client.fileWriteFd] = clientFd;
                    }

                    for (size_t k = 0; k < _pollFds.size(); ++k) {
                        if (_pollFds[k].fd == clientFd) {
                            _pollFds[k].events = POLLIN;
                            break;
                        }
                    }
                    return true;
                }
                // ==============================================================
                // NORMAL FLOW (In-memory pages, Errors, Redirects)
                // ==============================================================
                else {
                    client.responseBuffer = res.getRawResponse();
                    client.bytesSent = 0;
                    client.isReadyToSend = true;
                }
            }

            if (client.isReadyToSend) {
                size_t bytesRemaining = client.responseBuffer.length() - client.bytesSent;
                ssize_t sent = send(clientFd, client.responseBuffer.c_str() + client.bytesSent, bytesRemaining, 0);
                
                if (sent <= 0) {
                    std::cerr << "[ERRO] Falha ao enviar resposta ou conexao prematuramente fechada pelo cliente\n";
                    return false;
                }
                
                client.bytesSent += sent;
                client.updateActivity();
                
                std::cout << "[HTTP] Chunck enviado ao FD " << clientFd << " (Tamanho: " << sent << " bytes)\n";
                
                if (client.bytesSent >= client.responseBuffer.length()) {
                    std::cout << "[HTTP] Resposta completa enviada com sucesso ao FD " << clientFd << "\n";

                    if (!client.req.wantsKeepAlive()) {
                        return false; // HTTP/1.0 default, explicit Connection: close, or grave parsing error
                    }

                    // Keep-Alive: reset the Client's state to accept a new
                    // request on the SAME TCP connection, preserving any byte
                    // that already arrived pipelined (2nd request sent by the
                    // client without waiting for the 1st response).
                    std::string leftover = client.req.extractLeftoverRaw();
                    client = Client();
                    client.updateActivity();

                    if (!leftover.empty()) {
                        client.req.appendToRaw(leftover);
                        _parseClientRequest(clientFd);
                    }

                    for (size_t k = 0; k < _pollFds.size(); ++k) {
                        if (_pollFds[k].fd == clientFd) {
                            bool pipelinedReady = client.req.isComplete() && !client.isCgi && !client.isFile;
                            _pollFds[k].events = pipelinedReady ? POLLOUT : POLLIN;
                            break;
                        }
                    }
                    return true;
                }
                return true;
            }
        }
    }
    return true; 
}

void Server::_checkTimeouts() {
    time_t now = time(NULL);
    const double TIMEOUT_SECONDS = 60.0;

    for (size_t i = _pollFds.size(); i > 0; --i) {
        size_t idx = i - 1;
        int fd = _pollFds[idx].fd;

        // Skip timeout so we don't drop File I/O or a running CGI
        if (_isListenSocket(fd) || _cgiToClient.find(fd) != _cgiToClient.end() || _fileToClient.find(fd) != _fileToClient.end()) {
            continue; 
        }

        if (_clients.find(fd) != _clients.end()) {
            double elapsed = difftime(now, _clients[fd].lastActivity);
            if (elapsed > TIMEOUT_SECONDS) {
                std::cout << "[TIMEOUT] Derrubando conexao ociosa (Hanging Connection). FD: " << fd << "\n";
                _closeClient(fd, idx);
            }
        }
    }
}

// A stuck CGI (e.g. a script in an infinite loop) can't be left hanging
// forever: unlike the idle timeout above, here the client is "active"
// (waiting for the CGI to respond), so it needs its own limit based on how
// long the child process has been running.
void Server::_checkCgiTimeouts() {
    time_t now = time(NULL);
    const double CGI_TIMEOUT_SECONDS = 10.0;

    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        int clientFd = it->first;
        Client& client = it->second;

        if (!client.isCgi) continue;
        if (difftime(now, client.cgiStart) <= CGI_TIMEOUT_SECONDS) continue;

        std::cout << "[TIMEOUT] CGI (PID " << client.cgiPid << ") excedeu " << CGI_TIMEOUT_SECONDS
                   << "s. Encerrando processo travado. Cliente FD: " << clientFd << "\n";

        kill(client.cgiPid, SIGKILL);
        waitpid(client.cgiPid, NULL, 0);

        if (client.cgiReadFd != -1) {
            _erasePollFd(client.cgiReadFd);
            _cgiToClient.erase(client.cgiReadFd);
            close(client.cgiReadFd);
            client.cgiReadFd = -1;
        }
        if (client.cgiWriteFd != -1) {
            _erasePollFd(client.cgiWriteFd);
            _cgiToClient.erase(client.cgiWriteFd);
            close(client.cgiWriteFd);
            client.cgiWriteFd = -1;
        }

        client.isCgi = false;
        client.responseBuffer = "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n";
        client.bytesSent = 0;
        client.isReadyToSend = true;

        for (size_t k = 0; k < _pollFds.size(); ++k) {
            if (_pollFds[k].fd == clientFd) {
                _pollFds[k].events = POLLOUT;
                break;
            }
        }
    }
}

void Server::_runEventLoop() {
    std::cout << "\n[INFO] Servidor online. Aguardando conexoes...\n";

    while (true) {
        int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
        if (ret < 0) throw std::runtime_error("Erro: Falha na fun o poll()");

        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents == 0) continue;

            // =================================================================
            // 1. HANDLING CGI PIPES (Asynchronous Processing)
            // =================================================================
            if (_cgiToClient.find(_pollFds[i].fd) != _cgiToClient.end()) {
                int clientFd = _cgiToClient[_pollFds[i].fd];
                Client& client = _clients[clientFd];

                if (_pollFds[i].revents & (POLLERR | POLLNVAL)) {
                    if (_pollFds[i].fd == client.cgiReadFd) {
                        int status = 0;
                        waitpid(client.cgiPid, &status, WNOHANG);
                        close(client.cgiReadFd);
                        client.cgiReadFd = -1;
                        client.responseBuffer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                        client.isReadyToSend = true;
                        client.isCgi = false;
                        for (size_t k = 0; k < _pollFds.size(); ++k) {
                            if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
                        }
                    }
                    _cgiToClient.erase(_pollFds[i].fd);
                    _pollFds.erase(_pollFds.begin() + i);
                    i--;
                    continue;
                }

                if (_pollFds[i].fd == client.cgiWriteFd) {
                    if (_pollFds[i].revents & (POLLOUT | POLLHUP)) {
                        // Reference, not a copy: getBody() used to copy the whole
                        // body on every single POLLOUT event, turning a POST with
                        // a large body into O(n^2) work (a full-body copy per
                        // 4-8KB chunk written).
                        const std::string& body = client.req.getBody();
                        size_t remaining = body.length() - client.cgiBytesWritten;
                        ssize_t sent = 0;
                        
                        if (_pollFds[i].revents & POLLOUT) {
                            sent = write(_pollFds[i].fd, body.c_str() + client.cgiBytesWritten, remaining);
                            if (sent > 0) client.cgiBytesWritten += sent;
                        }
                        
                        if (client.cgiBytesWritten >= body.length() || sent <= 0 || (_pollFds[i].revents & POLLHUP)) {
                            close(_pollFds[i].fd); 
                            _cgiToClient.erase(_pollFds[i].fd);
                            _pollFds.erase(_pollFds.begin() + i);
                            client.cgiWriteFd = -1;
                            i--;
                        }
                    }
                    continue;
                }

                if (_pollFds[i].fd == client.cgiReadFd) {
                    if ((_pollFds[i].revents & POLLIN) || (_pollFds[i].revents & POLLHUP)) {
                        char buffer[4096];
                        ssize_t bytesRead = read(_pollFds[i].fd, buffer, sizeof(buffer) - 1);
                        
                        if (bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            client.cgiOutput += buffer;
                        }

                        if (bytesRead <= 0 || (_pollFds[i].revents & POLLHUP)) {
                            // Blocking on purpose: the pipe only gives EOF once the child
                            // process closes stdout, which happens when it exits — so the
                            // wait here returns almost instantly. Using WNOHANG at this
                            // point would risk reading status=0 (process not yet reaped)
                            // and mistaking that for "exited successfully".
                            int status = 0;
                            waitpid(client.cgiPid, &status, 0);
                            close(_pollFds[i].fd);
                            _cgiToClient.erase(_pollFds[i].fd);
                            _pollFds.erase(_pollFds.begin() + i);
                            client.cgiReadFd = -1;
                            i--;

                            bool cgiOk = WIFEXITED(status) && WEXITSTATUS(status) == 0;

                            if (!cgiOk) {
                                std::cerr << "[CGI] Script (PID " << client.cgiPid << ") terminou com falha (status "
                                           << status << "). Respondendo 500.\n";
                                client.responseBuffer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                            } else {
                                size_t headerEnd = client.cgiOutput.find("\r\n\r\n");
                                size_t headerSize = 4;
                                if (headerEnd == std::string::npos) {
                                    headerEnd = client.cgiOutput.find("\n\n");
                                    headerSize = 2;
                                }

                                std::string cgiHeaders = "";
                                std::string cgiBody = client.cgiOutput;

                                if (headerEnd != std::string::npos) {
                                    cgiHeaders = client.cgiOutput.substr(0, headerEnd);
                                    cgiBody = client.cgiOutput.substr(headerEnd + headerSize);
                                }

                                std::ostringstream responseStream;
                                responseStream << "HTTP/1.1 200 OK\r\n";
                                responseStream << "Content-Length: " << cgiBody.length() << "\r\n";
                                if (!cgiHeaders.empty()) responseStream << cgiHeaders << "\r\n\r\n";
                                else responseStream << "Content-Type: text/html\r\n\r\n";
                                responseStream << cgiBody;

                                client.responseBuffer = responseStream.str();
                            }

                            client.bytesSent = 0;
                            client.isReadyToSend = true;
                            client.isCgi = false;

                            for (size_t k = 0; k < _pollFds.size(); ++k) {
                                if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
                            }
                        }
                    }
                    continue;
                }
            }
            
            // =================================================================
            // 2. HANDLING DISK FILES (Asynchronous I/O)
            // =================================================================
            if (_fileToClient.find(_pollFds[i].fd) != _fileToClient.end()) {
                int clientFd = _fileToClient[_pollFds[i].fd];
                Client& client = _clients[clientFd];

                if (_pollFds[i].revents & (POLLERR | POLLNVAL)) {
                    close(_pollFds[i].fd);
                    _fileToClient.erase(_pollFds[i].fd);
                    _pollFds.erase(_pollFds.begin() + i);
                    client.fileReadFd = -1;
                    client.fileWriteFd = -1;
                    client.isFile = false;
                    client.responseBuffer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                    client.isReadyToSend = true;
                    for (size_t k = 0; k < _pollFds.size(); ++k) {
                        if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
                    }
                    i--;
                    continue;
                }

                // Reading from disk (GET or Error Pages)
                if (_pollFds[i].fd == client.fileReadFd && (_pollFds[i].revents & POLLIN)) {
                    char buffer[8192]; // Reads in chunks
                    ssize_t bytesRead = read(_pollFds[i].fd, buffer, sizeof(buffer));
                    
                    if (bytesRead > 0) {
                        client.responseBuffer.append(buffer, bytesRead);
                    }
                    
                    if (bytesRead <= 0) { // EOF (Read complete)
                        close(_pollFds[i].fd);
                        _fileToClient.erase(_pollFds[i].fd);
                        _pollFds.erase(_pollFds.begin() + i);
                        client.fileReadFd = -1;
                        client.isFile = false;
                        client.isReadyToSend = true;
                        client.bytesSent = 0;
                        
                        for (size_t k = 0; k < _pollFds.size(); ++k) {
                            if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
                        }
                        i--;
                    }
                    continue;
                }

                // Writing to disk (POST Upload)
                if (_pollFds[i].fd == client.fileWriteFd && (_pollFds[i].revents & POLLOUT)) {
                    // Reference, not a copy — same reasoning as the CGI write above.
                    const std::string& body = client.req.getBody();
                    size_t remaining = body.length() - client.fileBytesWritten;
                    ssize_t bytesWritten = write(_pollFds[i].fd, body.c_str() + client.fileBytesWritten, remaining);
                    
                    if (bytesWritten > 0) {
                        client.fileBytesWritten += bytesWritten;
                    }
                    
                    if (client.fileBytesWritten >= body.length() || bytesWritten <= 0) {
                        close(_pollFds[i].fd);
                        _fileToClient.erase(_pollFds[i].fd);
                        _pollFds.erase(_pollFds.begin() + i);
                        client.fileWriteFd = -1;
                        client.isFile = false;
                        client.isReadyToSend = true; // The 201 success header was already placed there by _handleClientWrite
                        client.bytesSent = 0;
                        
                        for (size_t k = 0; k < _pollFds.size(); ++k) {
                            if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
                        }
                        i--;
                    }
                    continue;
                }
            }

            // =================================================================
            // 3. NORMAL CLIENT AND NETWORK HANDLING
            // =================================================================
            if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::cout << "[REDE] Erro/HUP no cliente. FD: " << _pollFds[i].fd << "\n";
                _closeClient(_pollFds[i].fd, i);
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
                        _closeClient(_pollFds[i].fd, i);
                        i--;
                        continue;
                    } else {
                        if (_clients[_pollFds[i].fd].req.isComplete() && !_clients[_pollFds[i].fd].isCgi && !_clients[_pollFds[i].fd].isFile) {
                            _pollFds[i].events = POLLOUT;
                        }
                    }
                }
            }

            if (_pollFds[i].revents & POLLOUT) {
                if (!_isListenSocket(_pollFds[i].fd)) {
                    bool keepAlive = _handleClientWrite(_pollFds[i].fd);
                    if (!keepAlive) {
                        std::cout << "[REDE] Fechando conexao (fim da requisicao). FD: " << _pollFds[i].fd << "\n";
                        _closeClient(_pollFds[i].fd, i);
                        i--;
                        continue;
                    }
                }
            }
        }

        _checkTimeouts();
        _checkCgiTimeouts();
    }
}

void Server::start() {
    std::cout << "Inicializando servidor...\n";
    _setupSockets();
    _runEventLoop();
}