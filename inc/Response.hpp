#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <sstream>
#include <fstream>
#include "Request.hpp"

class Response {
private:
    std::string _rawResponse;
    
    std::string _getContentType(const std::string& path) const;

public:
    Response();
    ~Response();

    void build(const Request& req);
    
    std::string getRawResponse() const;
};

#endif