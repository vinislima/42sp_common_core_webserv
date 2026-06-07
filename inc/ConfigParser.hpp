#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

class ConfigParser {
private:
    std::string                 _configFile;
    std::vector<std::string>    _tokens;

    void    _tokenize(const std::string& content);

public:
    ConfigParser();
    ConfigParser(const std::string& filename);
    ConfigParser(const ConfigParser& src);
    ConfigParser& operator=(const ConfigParser& rhs);
    ~ConfigParser();

    void                        parse();
    std::vector<std::string>    getTokens() const;
};

#endif