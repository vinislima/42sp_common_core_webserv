/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 20:11:00 by vinda-si          #+#    #+#             */
/*   Updated: 2026/09/06 09:45:32 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <sstream>
#include <map>
#include "Request.hpp"
#include "ServerConfig.hpp"

class Response {
private:
    int _statusCode;
    std::string _body;
    std::string  _rawResponse;
    std::map<std::string, std::string> _headers;
    std::map<int, std::string> _statusMessages;
    std::string _getFallbackHTML(int code) const;
    
    pid_t _cgiPid;
    int _cgiReadFd;
    int _cgiWriteFd;
    
    // New variables
    int _fileReadFd;
    int _fileWriteFd;

    void _initStatusMessages();
    void _generateRawResponse();
    std::string _getContentType(const std::string& path) const;
    bool _normalizeUri(const std::string& uri, std::string& out) const;
    bool _parseMultipart(const std::string& body, const std::string& boundary,
                          std::string& outFilename, std::string& outContent) const;
    void _buildErrorPage(int code, const ServerConfig& config);
    void _handleCGI(const std::string& filepath, const std::string& cgiPath, const Request& req, const ServerConfig& config);

public:
    Response();
    ~Response();

    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void build(Request& req, const ServerConfig& config);
    
    pid_t getCgiPid() const { return _cgiPid; }
    int getCgiReadFd() const { return _cgiReadFd; }
    int getCgiWriteFd() const { return _cgiWriteFd; }
    
    // New getters
    int getFileReadFd() const { return _fileReadFd; }
    int getFileWriteFd() const { return _fileWriteFd; }
    
    std::string getRawResponse() const;
};

#endif