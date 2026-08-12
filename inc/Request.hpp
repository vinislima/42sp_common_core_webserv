#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <cstdlib>

class Request {
private:
    std::string _rawRequest;
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _isComplete;

    void _parseRequestLine(const std::string& line);
    void _parseHeaders(const std::string& headersBlock);
    void _parseBody(const std::string& bodyBlock);
    void _parseChunkedBody(const std::string& bodyBlock);

public:
    Request();
    Request(const std::string& rawRequest);
    Request(const Request& src);
    Request& operator=(const Request& rhs);
    ~Request();

    void parse();
    void appendToRaw(const std::string& data);
    bool isComplete() const;

    std::string getMethod() const;
    std::string getUri() const;
    std::string getVersion() const;
    std::string getBody() const;
    std::string getHeader(const std::string& key) const;
    std::map<std::string, std::string> getHeaders() const;
};

#endif