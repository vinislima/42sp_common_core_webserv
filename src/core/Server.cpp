#include "../../inc/Server.hpp"
#include "../../inc/Request.hpp"
#include "../../inc/Response.hpp" 
#include <sys/wait.h> // Para waitpid() no CGI
#include <cstdlib>    // Para std::strtoul

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
	client.updateActivity();
	
	try {
		if (!client.req.areHeadersParsed()) {
			client.req.parseHeadersOnly();
		}

		if (client.req.areHeadersParsed() && !client.req.isBodyAuthorized()) {
			std::string hostHeader = client.req.getHeader("Host");
			size_t colonPos = hostHeader.find(':');
			
			if(colonPos != std::string::npos) {
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
			size_t maxBodySize = matchedConfig->getClientMaxBodySize();
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
		if (client.req.areHeadersParsed() && client.req.getErrorCode() == 0) {
			client.req.parseBodyOnly();
		}
	} catch (const std::exception& e) {
		std::string errorMsg = e.what();
		
		// O TCP fragmentou o pacote. Mantemos a conexão viva e aguardamos o resto.
		if (errorMsg == "Body incompleto" || errorMsg == "Chunk incompleto") {
			return true; 
		}

		// Erro real de protocolo
		std::cerr << "[ERRO] Requisicao malformada ou invalida: " << errorMsg << "\n";
		client.req.setErrorCode(400);
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

				// ==============================================================
				// INTERCEPTADOR DE CGI: Transfere os FDs para o poll()
				// ==============================================================
				if (res.getCgiPid() != -1) {
					client.isCgi = true;
					client.cgiPid = res.getCgiPid();
					client.cgiReadFd = res.getCgiReadFd();
					client.cgiWriteFd = res.getCgiWriteFd();
					client.cgiOutput = "";
					client.cgiBytesWritten = 0;

					// Coloca o tubo de LEITURA na fila do poll()
					struct pollfd pfdRead;
					pfdRead.fd = client.cgiReadFd;
					pfdRead.events = POLLIN;
					pfdRead.revents = 0;
					_pollFds.push_back(pfdRead);
					_cgiToClient[client.cgiReadFd] = clientFd;

					// Se tiver Body (POST), coloca o de ESCRITA na fila
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

					// Retira o POLLOUT do cliente para o servidor parar de tentar responder
					for (size_t k = 0; k < _pollFds.size(); ++k) {
						if (_pollFds[k].fd == clientFd) {
							_pollFds[k].events = POLLIN;
							break;
						}
					}
					return true; // Volta para o Event Loop
				}

				// FLUXO NORMAL (Arquivos Estáticos)
				client.responseBuffer = res.getRawResponse();
				client.bytesSent = 0;
				client.isReadyToSend = true;
			}
			
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
				return false;
			}
			return true;
		}
	}
	return true; 
}

void Server::_checkTimeouts() {
	time_t now = time(NULL);
	const double TIMEOUT_SECONDS = 60.0;

	for (size_t i = _pollFds.size(); i > 0; --i) {
		size_t	idx = i - 1;
		int		fd = _pollFds[idx].fd;

		if (_isListenSocket(fd) || _cgiToClient.find(fd) != _cgiToClient.end()) {
			continue; // Não dá timeout em tubos do CGI nem em Listen Sockets
		}
		
		if (_clients.find(fd) != _clients.end()) {
			double elapsed = difftime(now, _clients[fd].lastActivity);

			if (elapsed > TIMEOUT_SECONDS) {
				std::cout << "[TIMEOUT] Derrubando conexao ociosa (Hanging Connection). FD: " << fd << "\n";

				close(fd);
				_clients.erase(fd);
				_pollFds.erase(_pollFds.begin() + idx);
			}
		}
	}
}

void Server::_runEventLoop() {
	std::cout << "\n[INFO] Servidor online. Aguardando conexões...\n";
	while (true) {
		int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
		if (ret < 0) throw std::runtime_error("Erro: Falha na função poll()");

		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].revents == 0) continue;

			// =================================================================
			// 1. TRATAMENTO DOS TUBOS DO CGI (Processamento Assíncrono)
			// =================================================================
			if (_cgiToClient.find(_pollFds[i].fd) != _cgiToClient.end()) {
				int clientFd = _cgiToClient[_pollFds[i].fd];
				Client& client = _clients[clientFd];

				// Tratamento de Erro fatal no Pipe (sem o POLLHUP aqui)
				if (_pollFds[i].revents & (POLLERR | POLLNVAL)) {
					if (_pollFds[i].fd == client.cgiReadFd) {
						waitpid(client.cgiPid, NULL, WNOHANG);
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

				// TUBO DE ESCRITA: Webserv empurrando o Body (POST) pro CGI
				if (_pollFds[i].fd == client.cgiWriteFd) {
					if (_pollFds[i].revents & (POLLOUT | POLLHUP)) {
						std::string body = client.req.getBody();
						size_t remaining = body.length() - client.cgiBytesWritten;
						ssize_t sent = 0;
						
						if (_pollFds[i].revents & POLLOUT) {
							sent = write(_pollFds[i].fd, body.c_str() + client.cgiBytesWritten, remaining);
							if (sent > 0) client.cgiBytesWritten += sent;
						}

						if (client.cgiBytesWritten >= body.length() || sent <= 0 || (_pollFds[i].revents & POLLHUP)) {
							close(_pollFds[i].fd); // Fechar envia o sinal de EOF
							_cgiToClient.erase(_pollFds[i].fd);
							_pollFds.erase(_pollFds.begin() + i);
							client.cgiWriteFd = -1;
							i--;
						}
					}
					continue;
				}

				// TUBO DE LEITURA: Webserv lendo o Output que o CGI gerou
				if (_pollFds[i].fd == client.cgiReadFd) {
					if ((_pollFds[i].revents & POLLIN) || (_pollFds[i].revents & POLLHUP)) {
						char buffer[4096];
						ssize_t bytesRead = read(_pollFds[i].fd, buffer, sizeof(buffer) - 1);

						if (bytesRead > 0) {
							buffer[bytesRead] = '\0';
							client.cgiOutput += buffer;
						} 
						
						// Se o read retornar 0, batemos no EOF real, ou o pipe quebrou sem dados.
						if (bytesRead <= 0 || (_pollFds[i].revents & POLLHUP)) {
							waitpid(client.cgiPid, NULL, WNOHANG); // Limpa o Processo Zumbi
							close(_pollFds[i].fd);
							_cgiToClient.erase(_pollFds[i].fd);
							_pollFds.erase(_pollFds.begin() + i);
							client.cgiReadFd = -1;
							i--;

							// Separa os Cabeçalhos do CGI do HTML gerado
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

							// Monta a Resposta HTTP Final baseada na saída do script
							std::ostringstream responseStream;
							responseStream << "HTTP/1.1 200 OK\r\n";
							responseStream << "Content-Length: " << cgiBody.length() << "\r\n";
							if (!cgiHeaders.empty()) responseStream << cgiHeaders << "\r\n\r\n";
							else responseStream << "Content-Type: text/html\r\n\r\n";
							responseStream << cgiBody;

							client.responseBuffer = responseStream.str();
							client.bytesSent = 0;
							client.isReadyToSend = true;
							client.isCgi = false;

							// Devolve o POLLOUT para o Cliente!
							for (size_t k = 0; k < _pollFds.size(); ++k) {
								if (_pollFds[k].fd == clientFd) _pollFds[k].events = POLLOUT;
							}
						}
					}
					continue;
				}
			}

			// =================================================================
			// 2. TRATAMENTO NORMAL DOS CLIENTES E REDE
			// =================================================================
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
					} else {
						if (_clients[_pollFds[i].fd].req.isComplete() && !_clients[_pollFds[i].fd].isCgi) {
							_pollFds[i].events = POLLOUT;
						}
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
		_checkTimeouts();
	}
}

void Server::start() {
	std::cout << "Inicializando servidor...\n";
	_setupSockets();
	_runEventLoop();
}