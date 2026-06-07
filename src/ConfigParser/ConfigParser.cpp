#include "../../inc/ConfigParser.hpp"
#include <cctype>


ConfigParser::ConfigParser() : _configFile("default.conf") {}

ConfigParser::ConfigParser(const std::string& filename) : _configFile(filename) {}

ConfigParser::ConfigParser(const ConfigParser& src) {
    *this = src;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& rhs) {
    if (this != &rhs) {
        this->_configFile = rhs._configFile;
        this->_tokens = rhs._tokens;
    }
    return *this;
}

ConfigParser::~ConfigParser() {}

void ConfigParser::parse() {
    std::ifstream       file(_configFile.c_str());
    std::stringstream   buffer;

    if (!file.is_open()) {
        throw std::runtime_error("Erro: Não foi possível abrir o arquivo de configuração: " + _configFile);
    }

    buffer << file.rdbuf();
    file.close();

    _tokenize(buffer.str());
}

void ConfigParser::_tokenize(const std::string& content) {
    std::string currentToken = "";

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (c == '#') {
            while (i < content.length() && content[i] != '\n') {
                i++;
            }
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!currentToken.empty()) {
                _tokens.push_back(currentToken);
                currentToken.clear();
            }
            continue;
        }

        if (c == '{' || c == '}' || c == ';') {
            if (!currentToken.empty()) {
                _tokens.push_back(currentToken);
                currentToken.clear();
            }
            _tokens.push_back(std::string(1, c));
            continue;
        }

        currentToken += c;
    }

    if (!currentToken.empty()) {
        _tokens.push_back(currentToken);
    }
}

std::vector<std::string> ConfigParser::getTokens() const {
    return _tokens;
}