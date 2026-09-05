#include "../../inc/Response.hpp"
#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>

Response::Response() : _statusCode(200), _cgiPid(-1), _cgiReadFd(-1), _cgiWriteFd(-1) {
	_initStatusMessages();
}

Response::~Response() {}

void Response::_initStatusMessages() {
	_statusMessages[200] = "OK";
	_statusMessages[201] = "Created";
	_statusMessages[204] = "No Content";
	_statusMessages[301] = "Moved Permanently";
	_statusMessages[400] = "Bad Request";
	_statusMessages[403] = "Forbidden";
	_statusMessages[404] = "Not Found";
	_statusMessages[405] = "Method Not Allowed";
	_statusMessages[413] = "Payload Too Large";
	_statusMessages[500] = "Internal Server Error";
	_statusMessages[501] = "Not Implemented";
}

std::string Response::_getContentType(const std::string& path) const {
	if (path.find(".html") != std::string::npos) return "text/html";
	if (path.find(".css") != std::string::npos) return "text/css";
	if (path.find(".png") != std::string::npos) return "image/png";
	return "text/plain";
}

void Response::setStatusCode(int code) {
	_statusCode = code;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
}

void Response::_generateRawResponse() {
	std::ostringstream ss;

	ss << "HTTP/1.1 " << _statusCode << " ";
	if (_statusMessages.find(_statusCode) != _statusMessages.end())
		ss << _statusMessages[_statusCode];
	else
		ss << "Unknown Status";
	ss << "\r\n";

	std::ostringstream lengthStream;
	lengthStream << _body.length();
	_headers["Content-Length"] = lengthStream.str();

	if (_headers.find("Content-Type") == _headers.end() && !_body.empty()) {
		_headers["Content-Type"] = "text/plain";
	}

	for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
		ss << it->first << ": " << it->second << "\r\n";
	}

	ss << "\r\n";

	ss << _body;

	_rawResponse = ss.str();
}

const LocationConfig* Response::_getBestMatchLocation(const std::string& uri, const ServerConfig& config) const {
	const	LocationConfig* bestMatch = NULL;
	size_t	longestMatchLength = 0;
	const	std::vector<LocationConfig>& location = config.getLocations();

	for (std::vector<LocationConfig>::const_iterator it = location.begin(); it != location.end(); ++it) {
		std::string locPath = it->getPath();

		if (uri.find(locPath) == 0) {
			if (locPath.length() > longestMatchLength) {
				longestMatchLength = locPath.length();
				bestMatch = &(*it);
			}
		}
	}
	return bestMatch;
}

std::string Response::_getFallbackHTML(int code) const {
	std::ostringstream	oss;
	std::string			msg = "Unknown Error";
	std::map<int, std::string>::const_iterator it = _statusMessages.find(code);

	if (it != _statusMessages.end()) {
		msg = it->second;
	}

	oss << "<!DOCTYPE html>\n<html>\n<head><title>" << code << " " << msg << "</title></head>\n"
		<< "<body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; text-align: center; padding: 50px;\">\n"
		<< "	<h1 style=\"color: #ff5555;\">" << code << " - " << msg << "</h1>\n"
		<< "	<hr>\n"
		<< "	<p>Webserv / C++98 Fallback</p>\n"
		<< "</body>\n</html>";

	return oss.str();
}

void Response::_buildErrorPage(int code, const ServerConfig& config) {
	setStatusCode(code);
	bool customPageLoaded = false;

	std::map<int, std::string> errorPages = config.getErrorPages();
	std::map<int, std::string>::const_iterator it = errorPages.find(code);

	if (it != errorPages.end()) {
		std::string errorUri = it->second;
		std::string root = config.getRoot();
		if (root.empty()) {
			root = "./www";
		}

		std::string filepath = root + errorUri;

		std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
		if (file.is_open()) {
			std::ostringstream ss;
			ss << file.rdbuf();
			setBody(ss.str());
			setHeader("Content-Type", _getContentType(filepath));
			file.close();
			customPageLoaded = true;
		} else {
			std::cout << "[WARN] Nao foi possivel carregar error_page customizada em: " << filepath << ". Usando fallback.\n";
		}
	}
	if (!customPageLoaded) {
		setBody(_getFallbackHTML(code));
		setHeader("Content-Type", "text/html");
	}
	_generateRawResponse();
}

void Response::build(const Request& req, const ServerConfig& config) {
	_headers.clear();
	_body.clear();
	_rawResponse.clear();

	if (req.getErrorCode() != 0) {
		_buildErrorPage(req.getErrorCode(), config);
		// setStatusCode(req.getErrorCode());
		
		// std::ostringstream oss;
		// if (req.getErrorCode() == 413) {
		// 	oss << "<html><body><h1 style='color:red'>413 - Payload Too Large</h1></body></html>";
		// } else {
		// 	oss << "<html><body><h1 style='color:red'>" << req.getErrorCode() << " - Erro na Requisicao</h1></body></html>";
		// }
		
		// setBody(oss.str());
		// setHeader("Content-Type", "text/html");
		// _generateRawResponse();
		return;
	}
	setStatusCode(200);
	
	std::string cleanUri = req.getUri();
    size_t qmPos = cleanUri.find('?');
    if (qmPos != std::string::npos) {
        cleanUri = cleanUri.substr(0, qmPos); 
    }

    const LocationConfig* loc = _getBestMatchLocation(cleanUri, config);
    std::string root = (loc && !loc->getRoot().empty()) ? loc->getRoot() : config.getRoot();
    bool autoindex = (loc) ? loc->getAutoindex() : config.getAutoindex();
    std::vector<std::string> indexFiles = (loc && !loc->getIndex().empty()) ? loc->getIndex() : config.getIndex();

    if (root.empty()) {
        root = "./www";
    }

    std::string filepath = root + cleanUri;

    if (filepath != root && filepath[filepath.length() - 1] == '/') {
        filepath = filepath.substr(0, filepath.length() - 1);
    }

    std::string cgiExt = (loc) ? loc->getCgiExt() : "";
    std::string cgiPath = (loc) ? loc->getCgiPath() : "";

    if (!cgiExt.empty() && filepath.length() >= cgiExt.length()) {
        if (filepath.substr(filepath.length() - cgiExt.length()) == cgiExt) {
            std::cout << "[CGI] Arquivo " << cgiExt << " detectado! Redirecionando para execucao.\n";
            _handleCGI(filepath, cgiPath, req, config);
            return;
        }
    }

    std::cout << "[DEBUG] Tentando ler do disco o caminho: " << filepath << "\n";

	if (req.getMethod() == "GET") {
		struct stat path_stat;
		if (stat(filepath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
			bool indexFound = false;

			for (size_t i = 0; i < indexFiles.size(); ++i) {
				std::string indexPath = filepath + "/" + indexFiles[i];

				if (access(indexPath.c_str(), F_OK) == 0) {
					filepath = indexPath;
					indexFound = true;
					break;
				}
			}
			if (!indexFound) {
				if (autoindex) {
					DIR* dir = opendir(filepath.c_str());
					
					if (dir != NULL) {
						std::string autoindexHtml = "<!DOCTYPE html><html><head><title>Index of " + req.getUri() + "</title></head>";
						autoindexHtml += "<body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; padding: 20px;\">";
						autoindexHtml += "<h1 style=\"color: #50fa7b;\">Index of " + req.getUri() + "</h1><hr><ul>";

						struct dirent* entry;
						while ((entry = readdir(dir)) != NULL) {
							std::string name = entry->d_name;
							if (name == ".")
								continue;

							std::string link = req.getUri();
							if (link[link.length() - 1] != '/') link += "/";
							link += name;

							std::string fullPath = filepath + "/" + name;
							struct stat item_stat;
							if (stat(fullPath.c_str(), &item_stat) == 0 && S_ISDIR(item_stat.st_mode)) {
								name += "/";
							}
							autoindexHtml += "<li style=\"margin: 5px 0;\"><a style=\"color: #8be9fd; text-decoration: none; font-size: 1.2rem;\" href=\"" + link + "\">" + name + "</a></li>";
						}
						closedir(dir);
						autoindexHtml += "</ul><hr><p style=\"color: #6272a4;\">Webserv / C++98</p></body></html>";

						setStatusCode(200);
						setBody(autoindexHtml);
						setHeader("Content-Type", "text/html");
						_generateRawResponse();
						return;
					} else {
						_buildErrorPage(403, config);
						// setStatusCode(403);
						// setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
						// setHeader("Content-Type", "text/html");
						// _generateRawResponse();
						return;
					}
				} else {
					_buildErrorPage(403, config);
					// setStatusCode(403);
					// setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
					// setHeader("Content-Type", "text/html");
					// _generateRawResponse();
					return;
				}
			}
		}

		std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
		if (file.is_open()) {
			std::ostringstream ss;
			ss << file.rdbuf();
			setStatusCode(200);
			setBody(ss.str());
			setHeader("Content-Type", _getContentType(filepath));
			file.close();
		} else {
			_buildErrorPage(404, config);
			return;
			// setStatusCode(404);
			// setBody("<html><body><h1 style='color:red;'>404 - Pagina Nao Encontrada (Not Found)</h1></body></html>");
			// setHeader("Content-Type", "text/html");
		}
	}
	else if (req.getMethod() == "POST") {
		struct stat path_stat;
		if (stat(filepath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
			filepath += "/upload_generico.txt";
		}
		std::ofstream outFile(filepath.c_str(), std::ios::out | std::ios::binary);
		if (outFile.is_open()) {
			outFile.write(req.getBody().c_str(), req.getBody().length());
			outFile.close();
			setStatusCode(201);
			setBody("<html><body><h1 style='color:green'>201 - Arquivo Criado com Sucesso!</h1></body></html>");
			setHeader("Content-Type", "text/html");
		} else {
			_buildErrorPage(500, config);
			return;
			// setStatusCode(500);
			// setBody("<html><body><h1 style='color:red'>500 - Erro Interno</h1></body></html>");
			// setHeader("Content-Type", "text/html");
		}
	}
	else if (req.getMethod() == "DELETE") {
		if (access(filepath.c_str(), F_OK) != 0) {
			_buildErrorPage(404, config);
			return;
			// setStatusCode(404);
			// setBody("<html><body><h1 style='color:red'>404 - Not Found</h1></body></html>");
			// setHeader("Content-Type", "text/html");
		}
		else if (std::remove(filepath.c_str()) == 0) {
			setStatusCode(200);
			setBody("<html><body><h1 style='color:green'>200 - OK (Deletado)</h1></body></html>");
			setHeader("Content-Type", "text/html");
		}
		else {
			_buildErrorPage(403, config);
			return;
			// setStatusCode(403);
			// setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
			// setHeader("Content-Type", "text/html");
		}
	}
	else {
		_buildErrorPage(405, config);
		return;
		// setStatusCode(405);
		// setBody("<html><body><h1>405 - Metodo Nao Permitido</h1></body></html>");
		// setHeader("Content-Type", "text/html");
	}
	_generateRawResponse();
}
std::string Response::getRawResponse() const {
	return _rawResponse;
}

void Response::_handleCGI(const std::string& filepath, const std::string& cgiPath, const Request& req, const ServerConfig& config) {
    std::string uri = req.getUri();
    std::string queryString = "";
    size_t qmPos = uri.find('?');
    if (qmPos != std::string::npos) {
        queryString = uri.substr(qmPos + 1);
        uri = uri.substr(0, qmPos);
    }

    std::string cleanFilepath = filepath;
    size_t filepathQM = cleanFilepath.find('?');
    if (filepathQM != std::string::npos) {
        cleanFilepath = cleanFilepath.substr(0, filepathQM);
    }

    std::string scriptDir = ".";
    std::string scriptName = cleanFilepath;
    size_t lastSlash = cleanFilepath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        scriptDir = cleanFilepath.substr(0, lastSlash);
        scriptName = cleanFilepath.substr(lastSlash + 1);
    }

    std::map<std::string, std::string> envMap;
    envMap["REQUEST_METHOD"] = req.getMethod();
    envMap["QUERY_STRING"] = queryString;
    envMap["SCRIPT_FILENAME"] = scriptName;
    envMap["SCRIPT_NAME"] = uri;
    envMap["SERVER_PROTOCOL"] = req.getVersion();
    envMap["SERVER_SOFTWARE"] = "Webserv/1.0";
    envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    envMap["REDIRECT_STATUS"] = "200";

    std::ostringstream portStream;
    portStream << config.getPort();
    envMap["SERVER_PORT"] = portStream.str();

    if (!req.getHeader("Content-Length").empty()) envMap["CONTENT_LENGTH"] = req.getHeader("Content-Length");
    else {
        std::ostringstream clStream; clStream << req.getBody().length();
        envMap["CONTENT_LENGTH"] = clStream.str();
    }
    if (!req.getHeader("Content-Type").empty()) envMap["CONTENT_TYPE"] = req.getHeader("Content-Type");

    char** envp = new char*[envMap.size() + 1];
    size_t i = 0;
    for (std::map<std::string, std::string>::iterator it = envMap.begin(); it != envMap.end(); ++it) {
        std::string envStr = it->first + "=" + it->second;
        envp[i] = new char[envStr.length() + 1];
        std::strcpy(envp[i], envStr.c_str());
        i++;
    }
    envp[i] = NULL;

    int pipeIn[2], pipeOut[2];
    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
        for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
        delete[] envp;
        _buildErrorPage(500, config);
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        close(pipeIn[0]); close(pipeIn[1]);
        close(pipeOut[0]); close(pipeOut[1]);
        for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
        delete[] envp;
        _buildErrorPage(500, config);
        return;
    }

    if (pid == 0) {
        if (chdir(scriptDir.c_str()) < 0) exit(1);

        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        close(pipeIn[0]); close(pipeIn[1]);
        close(pipeOut[0]); close(pipeOut[1]);

        char* argv[3];
        argv[0] = const_cast<char*>(cgiPath.c_str());
        argv[1] = const_cast<char*>(scriptName.c_str());
        argv[2] = NULL;

        execve(cgiPath.c_str(), argv, envp);
        exit(1);
    } else {
        close(pipeIn[0]);  // Pai não lê da entrada
        close(pipeOut[1]); // Pai não escreve na saída

        fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
        fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);

        this->_cgiPid = pid;
        this->_cgiWriteFd = pipeIn[1];
        this->_cgiReadFd = pipeOut[0];

        for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
        delete[] envp;

    }
}