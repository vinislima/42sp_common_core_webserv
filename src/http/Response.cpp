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
	_statusCode[200] = "OK";
	_statusCode[201] = "Created";
	_statusCode[204] = "No Content";
	_statusCode[301] = "Moved Permanently";
	_statusCode[400] = "Bad Request";
	_statusCode[403] = "Forbidden";
	_statusCode[404] = "Not Found";
	_statusCode[405] = "Method Not Allowed";
	_statusCode[413] = "Payload Too Large";
	_statusCode[500] = "Internal Server Error";
	_statusCode[501] = "Not Implemented";
}

std::string Response::_getContentType(const std::string& path) const {
	if (path.find(".html") != std::string::npos) return "text/html";
	if (path.find(".css") != std::string::npos) return "text/css";
	if (path.find(".png") != std::string::npos) return "image/png";
	return "text/plain";
}

void Response::statusCode(int code) {
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

	ss << "HTTP/1.1" << _statusCode << " ";
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

void Response::build(const Request& req) {
	_headers.clear();
	_body.clear();
	_rawResponse.clear();
	setStatusCode(200);

	if (req.getMethod() == "GET") {
		std::string filepath = "./www/" + req.getUri();

		if (filepath != "./www/" && filepath[filepath.length() - 1] == '/') {
			filepath = filepath.substr(0, filepath.length() - 1);
		}

		struct stat path_stat;
		if (stat(filepath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
			std::string indexPath = filepath + "/index.html";

			if (access(indexPath.c_str(), F_OK) == 0) {
				filepath = indexPath;
			} else {
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
			setHeader("Content-Type", "text/htl");
		}
	}
	else if (req.getMethod() == "POST") {
		std::string filepath = "./www" + req.getUri();
		if (req.getUri() == "/")
			filepath = "./www/upload_generico.txt";

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
		std::string filepath = "./www" + req.getUri();

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
