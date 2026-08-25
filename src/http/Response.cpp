#include "../../inc/Response.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

Response::Response() : _statusCode(200) {
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

void Response::build(const Request& req, const ServerConfig& config) {
	_headers.clear();
	_body.clear();
	_rawResponse.clear();

	if (req.getErrorCode() != 0) {
		setStatusCode(req.getErrorCode());
		
		std::ostringstream oss;
		if (req.getErrorCode() == 413) {
			oss << "<html><body><h1 style='color:red'>413 - Payload Too Large</h1></body></html>";
		} else {
			oss << "<html><body><h1 style='color:red'>" << req.getErrorCode() << " - Erro na Requisicao</h1></body></html>";
		}
		
		setBody(oss.str());
		setHeader("Content-Type", "text/html");
		_generateRawResponse();
		return;
	}
	setStatusCode(200);

	const LocationConfig* loc = _getBestMatchLocation(req.getUri(), config);

	std::string root = (loc && !loc->getRoot().empty()) ? loc->getRoot() : config.getRoot();
	bool autoindex = (loc) ? loc->getAutoindex() : config.getAutoindex();
	std::vector<std::string> indexFiles = (loc && !loc->getIndex().empty()) ? loc->getIndex() : config.getIndex();

	if (root.empty()) {
		root = "./www";
	}

	std::string filepath = root + req.getUri();

	if (filepath != root && filepath[filepath.length() - 1] == '/') {
		filepath = filepath.substr(0, filepath.length() - 1);
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
						setStatusCode(403);
						setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
						setHeader("Content-Type", "text/html");
						_generateRawResponse();
						return;
					}
				} else {
					setStatusCode(403);
					setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
					setHeader("Content-Type", "text/html");
					_generateRawResponse();
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
			setStatusCode(404);
			setBody("<html><body><h1 style='color:red;'>404 - Pagina Nao Encontrada (Not Found)</h1></body></html>");
			setHeader("Content-Type", "text/html");
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
			setStatusCode(500);
			setBody("<html><body><h1 style='color:red'>500 - Erro Interno</h1></body></html>");
			setHeader("Content-Type", "text/html");
		}
	}
	else if (req.getMethod() == "DELETE") {
		if (access(filepath.c_str(), F_OK) != 0) {
			setStatusCode(404);
			setBody("<html><body><h1 style='color:red'>404 - Not Found</h1></body></html>");
			setHeader("Content-Type", "text/html");
		}
		else if (std::remove(filepath.c_str()) == 0) {
			setStatusCode(200);
			setBody("<html><body><h1 style='color:green'>200 - OK (Deletado)</h1></body></html>");
			setHeader("Content-Type", "text/html");
		}
		else {
			setStatusCode(403);
			setBody("<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>");
			setHeader("Content-Type", "text/html");
		}
	}
	else {
		setStatusCode(405);
		setBody("<html><body><h1>405 - Metodo Nao Permitido</h1></body></html>");
		setHeader("Content-Type", "text/html");
	}
	_generateRawResponse();
}
std::string Response::getRawResponse() const {
	return _rawResponse;
}
