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
			std::cout << "[INFO] Socket para " << currentAddr << " ja está aberto. Configurado como Virtual Host.\n";
			continue;
		}
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			throw std::runtime_error("Erro: Falha ao criar o socket.");

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
			throw std::runtime_error("Erro: Falha no bind no endereço " + currentAddr);

		if (listen(sockfd, 128) < 0)
			throw std::runtime_error("Erro: Falha no listen.");

		_listenSockets.push_back(sockfd);

		struct pollfd pfd;
		pfd.fd = sockfd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_pollFds.push_back(pfd);

		boundAddresses.push_back(currentAddr);

		std::cout << "[SUCESSO] Servidor ouvindo em " << currentAddr << "\n";
	}
	// for (size_t i = 0; i < _configs.size(); ++i) {
	// 	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// 	if (sockfd < 0) throw std::runtime_error("Erro: Falha ao criar o socket.");
	// 	int opt = 1;
	// 	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) throw std::runtime_error("Erro: Falha no setsockopt.");
	// 	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) throw std::runtime_error("Erro: Falha ao definir socket como n o bloqueante.");
	// 	if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) < 0) throw std::runtime_error("Erro: Falha ao definir socket com FD_CLOEXEC.");
	// 	struct sockaddr_in addr;
	// 	std::memset(&addr, 0, sizeof(addr));
	// 	addr.sin_family = AF_INET;
	// 	addr.sin_port = htons(_configs[i].getPort());
	// 	addr.sin_addr.s_addr = inet_addr(_configs[i].getHost().c_str());
	// 	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) throw std::runtime_error("Erro: Falha no bind.");
	// 	if (listen(sockfd, 128) < 0) throw std::runtime_error("Erro: Falha no listen.");
	// 	_listenSockets.push_back(sockfd);
	// 	struct pollfd pfd;
	// 	pfd.fd = sockfd;
	// 	pfd.events = POLLIN;
	// 	pfd.revents = 0;
	// 	_pollFds.push_back(pfd);
	// 	std::cout << "[SUCESSO] Servidor ouvindo em " << _configs[i].getHost() << ":" << _configs[i].getPort() << "\n";
	// }
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
	if (bytesRead <= 0)
		return false;
	buffer[bytesRead] = '\0';
	Client& client = _clients[clientFd];
	client.req.appendToRaw(std::string(buffer, bytesRead));
	try {
		client.req.parse();
	} catch (const std::exception& e) {
		std::cout << "[DEBUG] Parse pendente ou erro: " << e.what() << "\n";
		return true; 
	}
	return true;
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
				const ServerConfig* matchedConfig = &_configs[0];

				for (size_t i = 0; i < _configs.size(); ++i) {
					std::vector<std::string> names = _configs[i].getServerNames();
					bool found = false;
					for (size_t j = 0; j < names.size(); ++j) {
						if (names[j] == hostHeader) {
							found = true;
							break;
						}
					}
					if (found) {
						matchedConfig = &_configs[i];
						break;
					}
				}
				res.build(client.req, *matchedConfig);
				client.responseBuffer = res.getRawResponse();
				client.bytesSent = 0;
				client.isReadyToSend = true;
			}
			size_t bytesRemaining = client.responseBuffer.length() - client.bytesSent;
			size_t sent = send(clientFd, client.responseBuffer.c_str() + client.bytesSent, bytesRemaining, 0);

			if (sent <= 0) {
				std::cerr << "[ERRO] Falha ao enviar resposta ou conexao prematuramente fechada pelo cliente\n";
				return false;
			}

			client.bytesSent += sent;
			std::cout << "[HTTP] Chunck enviado ao FD " << clientFd << "(Tamanho: " << sent << " bytes)\n";

			if (client.bytesSent >= client.responseBuffer.length()) {
			std::cout << "[HTTP] Resposta completa enviada com sucesso ao FD " << clientFd << "\n";
				return false;
			}
			return true;
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
				_clients.erase(_pollFds[i].fd); 
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
						_clients.erase(_pollFds[i].fd);
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
						_clients.erase(_pollFds[i].fd); 
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