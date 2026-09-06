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
#include <cctype>

Response::Response() : _statusCode(200), _cgiPid(-1), _cgiReadFd(-1), _cgiWriteFd(-1), _fileReadFd(-1), _fileWriteFd(-1) {
    _initStatusMessages();
}

Response::~Response() {}

void Response::_initStatusMessages() {
    _statusMessages[200] = "OK";
    _statusMessages[201] = "Created";
    _statusMessages[204] = "No Content";
    _statusMessages[301] = "Moved Permanently";
    _statusMessages[302] = "Found";
    _statusMessages[303] = "See Other";
    _statusMessages[307] = "Temporary Redirect";
    _statusMessages[308] = "Permanent Redirect";
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
    const LocationConfig* bestMatch = NULL;
    size_t longestMatchLength = 0;
    
    const std::vector<LocationConfig>& location = config.getLocations();
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
    std::ostringstream oss;
    std::string msg = "Unknown Error";
    std::map<int, std::string>::const_iterator it = _statusMessages.find(code);
    if (it != _statusMessages.end()) {
        msg = it->second;
    }
    oss << "<!DOCTYPE html>\n<html>\n<head><title>" << code << " " << msg << "</title></head>\n"
        << "<body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; text-align: center; padding: 50px;\">\n"
        << "  <h1 style=\"color: #ff5555;\">" << code << " - " << msg << "</h1>\n"
        << "  <hr>\n"
        << "  <p>Webserv / C++98 Fallback</p>\n"
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
        
        // I/O Assíncrono para página de erro!
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd >= 0) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
            this->_fileReadFd = fd;
            
            struct stat path_stat;
            stat(filepath.c_str(), &path_stat);
            
            std::ostringstream ss;
            ss << "HTTP/1.1 " << code << " " << _statusMessages[code] << "\r\n";
            ss << "Content-Length: " << path_stat.st_size << "\r\n";
            ss << "Content-Type: " << _getContentType(filepath) << "\r\n\r\n";
            _rawResponse = ss.str();
            customPageLoaded = true;
        } else {
            std::cout << "[WARN] Nao foi possivel carregar error_page customizada em: " << filepath << ". Usando fallback.\n";
        }
    }
    
    if (!customPageLoaded) {
        setBody(_getFallbackHTML(code));
        setHeader("Content-Type", "text/html");
        _generateRawResponse();
    }
}

void Response::build(const Request& req, const ServerConfig& config) {
    _headers.clear();
    _body.clear();
    _rawResponse.clear();

    if (req.getErrorCode() != 0) {
        _buildErrorPage(req.getErrorCode(), config);
        return;
    }

    setStatusCode(200);

    std::string cleanUri = req.getUri();
    size_t queryPos = cleanUri.find('?');
    if (queryPos != std::string::npos) {
        cleanUri = cleanUri.substr(0, queryPos);
    }

    const LocationConfig* loc = _getBestMatchLocation(cleanUri, config);

	if (loc && !loc->isMethodAllowed(req.getMethod())) {
        _buildErrorPage(405, config);
        return;
    }

    if (loc && loc->getRedirectCode() != 0) {
        int rCode = loc->getRedirectCode();
        setStatusCode(rCode);
        setHeader("Location", loc->getRedirectUrl());
        std::ostringstream oss;
        oss << "<html>\n<head><title>" << rCode << " " << _statusMessages[rCode] << "</title></head>\n"
            << "<body style=\"background-color: #282a36; color: #f8f8f2; text-align: center; padding: 50px;\">\n"
            << "  <h1>" << rCode << " - " << _statusMessages[rCode] << "</h1>\n"
            << "  <p>Redirecionando para <a style=\"color: #8be9fd;\" href=\"" << loc->getRedirectUrl() << "\">" 
            << loc->getRedirectUrl() << "</a></p>\n"
            << "</body>\n</html>";
        setBody(oss.str());
        setHeader("Content-Type", "text/html");
        _generateRawResponse();
        return;
    }

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
                            if (name == ".") continue;
                            
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
                        return;
                    }
                } else {
                    _buildErrorPage(403, config);
                    return;
                }
            }
        }
        
        // ==========================================
        // I/O ASSÍNCRONO: Abertura de Arquivo (GET)
        // ==========================================
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd >= 0) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
            this->_fileReadFd = fd;
            
            struct stat file_stat;
            stat(filepath.c_str(), &file_stat);
            
            std::ostringstream ss;
            ss << "HTTP/1.1 200 OK\r\n";
            ss << "Content-Length: " << file_stat.st_size << "\r\n";
            ss << "Content-Type: " << _getContentType(filepath) << "\r\n\r\n";
            _rawResponse = ss.str();
        } else {
            _buildErrorPage(404, config);
            return;
        }

    } else if (req.getMethod() == "POST") {
        std::string uploadStore = (loc) ? loc->getUploadStore() : "";
        if (uploadStore.empty()) {
            std::cout << "[DEBUG] Post recusado: Nenhum upload_store configurado para a rota " << req.getUri() << ".\n";
            _buildErrorPage(403, config);
            return;
        }
        if (uploadStore[uploadStore.length() - 1] == '/') {
            uploadStore = uploadStore.substr(0, uploadStore.length() - 1);
        }
        
        struct stat uploadStat;
        if (stat(uploadStore.c_str(), &uploadStat) != 0 || !S_ISDIR(uploadStat.st_mode)) {
            std::cout << "[ERRO] POST falhou: Diretorio de upload (" << uploadStore << ") nao existe no disco.\n";
            _buildErrorPage(500, config);
            return;
        }

        std::string filename = "";
        std::string contentDisposition = req.getHeader("Content-Disposition");
        if (!contentDisposition.empty()) {
            size_t namePos = contentDisposition.find("filename=\"");
            if (namePos != std::string::npos) {
                namePos += 10;
                size_t endPos = contentDisposition.find("\"", namePos);
                if (endPos != std::string::npos) {
                    filename = contentDisposition.substr(namePos, endPos - namePos);
                }
            }
        }
        if (filename.empty()) {
            std::ostringstream oss;
            oss << "upload_" << time(NULL) << ".bin";
            filename = oss.str();
        }
        
        std::string fullUploadPath = uploadStore + "/" + filename;
        
        // ==========================================
        // I/O ASSÍNCRONO: Abertura de Arquivo (POST)
        // ==========================================
        int fd = open(fullUploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
            this->_fileWriteFd = fd;
            
            std::string locationHeader = req.getUri();
            if (locationHeader[locationHeader.length() - 1] != '/') {
                locationHeader += "/";
            }
            locationHeader += filename;
            
            std::ostringstream responseHtml;
            responseHtml << "<html><body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; padding: 20px;\">\n"
                         << "<h1 style='color: #50fa7b;'>201 - Arquivo Salvo (Created) via poll()</h1>\n"
                         << "<p>Caminho no disco: " << fullUploadPath << "</p>\n"
                         << "</body></html>";
                         
            std::ostringstream ss;
            ss << "HTTP/1.1 201 Created\r\n";
            ss << "Location: " << locationHeader << "\r\n";
            ss << "Content-Length: " << responseHtml.str().length() << "\r\n";
            ss << "Content-Type: text/html\r\n\r\n";
            ss << responseHtml.str();
            _rawResponse = ss.str(); // Headers + Body de resposta pronto, o Server.cpp envia assim que o HD terminar de gravar.
        } else {
            std::cout << "[ERRO] Permissao negada ou disco cheio ao gravar em " << fullUploadPath << "\n";
            _buildErrorPage(500, config);
            return;
        }

    } else if (req.getMethod() == "DELETE") {
        if (access(filepath.c_str(), F_OK) != 0) {
            _buildErrorPage(404, config);
            return;
        } else if (std::remove(filepath.c_str()) == 0) {
            setStatusCode(200);
            setBody("<html><body><h1 style='color:green'>200 - OK (Deletado)</h1></body></html>");
            setHeader("Content-Type", "text/html");
        } else {
            _buildErrorPage(403, config);
            return;
        }
        _generateRawResponse();
    } else {
        _buildErrorPage(405, config);
        return;
    }
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

    std::map<std::string, std::string> clientHeaders = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = clientHeaders.begin(); it != clientHeaders.end(); ++it) {
        std::string key = "HTTP_";
        for (size_t k = 0; k < it->first.length(); ++k) {
            if (it->first[k] == '-') {
                key += '_';
            } else {
                key += std::toupper(static_cast<unsigned char>(it->first[k]));
            }
        }
        envMap[key] = it->second;
    }

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
        if (chdir(scriptDir.c_str()) < 0) {
            std::cerr << "[CGI FATAL] Falha no chdir para a pasta: " << scriptDir << "\n";
            exit(1);
        }
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        close(pipeIn[0]); close(pipeIn[1]);
        close(pipeOut[0]); close(pipeOut[1]);

        char* argv[3];
        argv[0] = const_cast<char*>(cgiPath.c_str());
        argv[1] = const_cast<char*>(scriptName.c_str());
        argv[2] = NULL;

        execve(cgiPath.c_str(), argv, envp);
        std::cerr << "[CGI FATAL] execve falhou ao tentar abrir o interpretador em: " << cgiPath << "\n";
        exit(1);
    } else {
        close(pipeIn[0]);
        close(pipeOut[1]);
        fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
        fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
        
        this->_cgiPid = pid;
        this->_cgiWriteFd = pipeIn[1];
        this->_cgiReadFd = pipeOut[0];
        
        for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
        delete[] envp;
    }
}