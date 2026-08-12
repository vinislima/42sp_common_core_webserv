#include "../../inc/Response.hpp"
#include <iostream>
#include <cstdio>     
#include <unistd.h>   
#include <sys/stat.h> 
#include <dirent.h>   

Response::Response() {}
Response::~Response() {}

std::string Response::_getContentType(const std::string& path) const {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".png") != std::string::npos) return "image/png";
    return "text/plain";
}

void Response::build(const Request& req) {
    
    if (req.getMethod() == "GET") {
        std::string filepath = "./www" + req.getUri();
        
        if (filepath != "./www/" && filepath[filepath.length() - 1] == '/') {
            filepath = filepath.substr(0, filepath.length() - 1);
        }

        struct stat path_stat;
        if (stat(filepath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
            
            std::string indexPath = filepath + "/index.html";
            
            if (access(indexPath.c_str(), F_OK) == 0) {
                filepath = indexPath; 
            } 
            else {
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

                        autoindexHtml += "<li style=\"margin: 5px 0;\"><a style=\"color: #8be9fd; text-decoration: none; font-size: 1.2rem;\" href=\"" + link + "\">📄 " + name + "</a></li>";
                    }
                    closedir(dir);
                    autoindexHtml += "</ul><hr><p style=\"color: #6272a4;\">Webserv / C++98</p></body></html>";

                    std::ostringstream responseStream;
                    responseStream << "HTTP/1.1 200 OK\r\n";
                    responseStream << "Content-Length: " << autoindexHtml.length() << "\r\n";
                    responseStream << "Content-Type: text/html\r\n\r\n";
                    responseStream << autoindexHtml;
                    
                    _rawResponse = responseStream.str();
                    return;
                }
            }
        }

        std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
        
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string body = ss.str();
            
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 200 OK\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: " << _getContentType(filepath) << "\r\n\r\n";
            responseStream << body;
            
            _rawResponse = responseStream.str();
            file.close();
        } else {
            std::string body = "<html><body><h1 style='color:red;'>404 - Pagina Nao Encontrada (Not Found)</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 404 Not Found\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            
            _rawResponse = responseStream.str();
        }
    }
    else if (req.getMethod() == "POST") {
        std::string filepath = "./www" + req.getUri();
        
        if (req.getUri() == "/") filepath = "./www/upload_generico.txt";

        std::ofstream outFile(filepath.c_str(), std::ios::out | std::ios::binary);
        
        if (outFile.is_open()) {
            outFile.write(req.getBody().c_str(), req.getBody().length());
            outFile.close();
            
            std::string body = "<html><body><h1 style='color:green'>201 - Arquivo Criado com Sucesso!</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 201 Created\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            
            _rawResponse = responseStream.str();
        } else {
            std::string body = "<html><body><h1 style='color:red'>500 - Erro Interno</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 500 Internal Server Error\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            _rawResponse = responseStream.str();
        }
    }
    else if (req.getMethod() == "DELETE") {
        std::string filepath = "./www" + req.getUri();

        if (access(filepath.c_str(), F_OK) != 0) {
            std::string body = "<html><body><h1 style='color:red'>404 - Not Found</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 404 Not Found\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            _rawResponse = responseStream.str();
        }
        else if (std::remove(filepath.c_str()) == 0) {
            std::string body = "<html><body><h1 style='color:green'>200 - OK (Deletado)</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 200 OK\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            _rawResponse = responseStream.str();
        }
        else {
            std::string body = "<html><body><h1 style='color:red'>403 - Forbidden</h1></body></html>";
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 403 Forbidden\r\n";
            responseStream << "Content-Length: " << body.length() << "\r\n";
            responseStream << "Content-Type: text/html\r\n\r\n";
            responseStream << body;
            _rawResponse = responseStream.str();
        }
    }
    else {
        std::string body = "<html><body><h1>405 - Metodo Nao Permitido</h1></body></html>";
        std::ostringstream responseStream;
        responseStream << "HTTP/1.1 405 Method Not Allowed\r\n";
        responseStream << "Content-Length: " << body.length() << "\r\n";
        responseStream << "Content-Type: text/html\r\n\r\n";
        responseStream << body;
        _rawResponse = responseStream.str();
    }
}

std::string Response::getRawResponse() const {
    return _rawResponse;
}