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
        this->_servers = rhs._servers;
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
    _buildTree();
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

void ConfigParser::_buildTree() {
    if (_tokens.empty()) {
        throw std::runtime_error("Erro: Arquivo de configuração vazio.");
    }

    int braceCount = 0;
    for (size_t k = 0; k < _tokens.size(); ++k) {
        if (_tokens[k] == "{") braceCount++;
        else if (_tokens[k] == "}") braceCount--;
        if (braceCount < 0) throw std::runtime_error("Erro de Sintaxe: Chave de fechamento '}' extra encontrada.");
    }
    if (braceCount > 0) throw std::runtime_error("Erro de Sintaxe: Faltam chaves de fechamento '}' no arquivo.");

    for (size_t i = 0; i < _tokens.size(); ++i) {
        if (_tokens[i] == "server") {
            i++; 
            if (i >= _tokens.size() || _tokens[i] != "{") {
                throw std::runtime_error("Erro: Bloco server sem chave de abertura '{'");
            }
            i++; 
            ServerConfig newServer;
            
            while (i < _tokens.size() && _tokens[i] != "}") {
                if (_tokens[i] == ";") {
                    i++; 
                    continue;
                }
                else if (_tokens[i] == "listen") {
                    _parseListen(newServer, i);
                }
                else if (_tokens[i] == "server_name") {
                    _parseServerName(newServer, i);
                }
                else if (_tokens[i] == "error_page") {
                    _parseErrorPage(newServer, i);
                }
                else if (_tokens[i] == "client_max_body_size") {
                    _parseClientMaxBodySize(newServer, i);
                }
                else if (_tokens[i] == "root") {
                    std::string rootVal;
                    _parseRoot(rootVal, i);
                    newServer.setRoot(rootVal);
                }
                else if (_tokens[i] == "autoindex") {
                    bool autoindexVal;
                    _parseAutoindex(autoindexVal, i);
                    newServer.setAutoindex(autoindexVal);
                }
                else if (_tokens[i] == "index") {
                    std::vector<std::string> indexList;
                    _parseIndex(indexList, i);
                    for (size_t j = 0; j < indexList.size(); ++j) {
                        newServer.addIndex(indexList[j]);
                    }
                }
                else if (_tokens[i] == "location") {
                    _parseLocation(newServer, i);
                }
                else {
                    throw std::runtime_error("Erro de Sintaxe: Diretiva desconhecida no bloco server: '" + _tokens[i] + "'");
                }
            }
            
            if (i == _tokens.size()) {
                throw std::runtime_error("Erro: Bloco server sem chave de fechamento '}'");
            }

            _servers.push_back(newServer);
        }
        else {
            throw std::runtime_error("Erro de Sintaxe: Diretiva global inválida (esperado 'server'): '" + _tokens[i] + "'");
        }
    }
}

void ConfigParser::_parseListen(ServerConfig& server, size_t& i) {
    i++; 
    if (i >= _tokens.size() || _tokens[i] == ";") {
         throw std::runtime_error("Erro: Diretiva listen vazia");
    }

    std::string value = _tokens[i];
    size_t colonPos = value.find(':');

    if (colonPos != std::string::npos) {
        std::string host = value.substr(0, colonPos);
        std::string portStr = value.substr(colonPos + 1);
        
        server.setHost(host);
        server.setPort(std::atoi(portStr.c_str()));
    } else {
        if (std::isdigit(value[0])) {
            server.setPort(std::atoi(value.c_str()));
        } else {
            server.setHost(value);
        }
    }

    i++; 
    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva listen sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseServerName(ServerConfig& server, size_t& i) {
    i++;
    
    while (i < _tokens.size() && _tokens[i] != ";") {
        server.addServerName(_tokens[i]);
        i++;
    }

    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva server_name sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseErrorPage(ServerConfig& server, size_t& i) {
    i++; 
    std::vector<int> codes;
    
    while (i < _tokens.size() && _tokens[i] != ";") {
        bool isNumber = true;
        for (size_t j = 0; j < _tokens[i].length(); ++j) {
            if (!std::isdigit(_tokens[i][j])) {
                isNumber = false;
                break;
            }
        }
        
        if (isNumber) {
            codes.push_back(std::atoi(_tokens[i].c_str()));
        } else {
            std::string uri = _tokens[i];
            for (size_t j = 0; j < codes.size(); ++j) {
                server.addErrorPage(codes[j], uri);
            }
        }
        i++;
    }
    
    if (i >= _tokens.size() || _tokens[i] != ";") {
        throw std::runtime_error("Erro: Diretiva error_page sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseClientMaxBodySize(ServerConfig& server, size_t& i) {
    i++; 
    if (i >= _tokens.size() || _tokens[i] == ";") {
         throw std::runtime_error("Erro: Diretiva client_max_body_size vazia");
    }
    
    std::string val = _tokens[i];
    size_t multiplier = 1;
    char lastChar = val[val.length() - 1];
    
    if (lastChar == 'm' || lastChar == 'M') {
        multiplier = 1024 * 1024;
        val = val.substr(0, val.length() - 1);
    } else if (lastChar == 'k' || lastChar == 'K') {
        multiplier = 1024;
        val = val.substr(0, val.length() - 1);
    } else if (lastChar == 'g' || lastChar == 'G') {
        multiplier = 1024 * 1024 * 1024;
        val = val.substr(0, val.length() - 1);
    }
    
    for (size_t j = 0; j < val.length(); ++j) {
        if (!std::isdigit(val[j])) {
            throw std::runtime_error("Erro: Valor inválido em client_max_body_size");
        }
    }
    
    std::stringstream ss(val);
    size_t size;
    ss >> size;
    server.setClientMaxBodySize(size * multiplier);
    
    i++;
    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva client_max_body_size sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseRoot(std::string& outRoot, size_t& i) {
    i++; 
    if (i >= _tokens.size() || _tokens[i] == ";") {
         throw std::runtime_error("Erro: Diretiva root vazia");
    }
    outRoot = _tokens[i];
    i++; 
    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva root sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseAutoindex(bool& outAutoindex, size_t& i) {
    i++; 
    if (i >= _tokens.size() || _tokens[i] == ";") {
         throw std::runtime_error("Erro: Diretiva autoindex vazia");
    }
    
    if (_tokens[i] == "on") {
        outAutoindex = true;
    } else if (_tokens[i] == "off") {
        outAutoindex = false;
    } else {
        throw std::runtime_error("Erro: Diretiva autoindex deve ser 'on' ou 'off'");
    }
    
    i++; 
    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva autoindex sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseIndex(std::vector<std::string>& outIndexList, size_t& i) {
    i++;
    while (i < _tokens.size() && _tokens[i] != ";") {
        outIndexList.push_back(_tokens[i]);
        i++;
    }
    if (i >= _tokens.size() || _tokens[i] != ";") {
        throw std::runtime_error("Erro: Diretiva index sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseLocation(ServerConfig& server, size_t& i) {
    i++; 
    
    if (i >= _tokens.size() || _tokens[i] == "{") {
         throw std::runtime_error("Erro: Bloco location sem caminho especificado");
    }
    
    std::string path = _tokens[i];
    i++; 
    
    if (i >= _tokens.size() || _tokens[i] != "{") {
         throw std::runtime_error("Erro: Bloco location sem chave de abertura '{'");
    }
    i++; 
    
    LocationConfig newLocation(path);
    
    while (i < _tokens.size() && _tokens[i] != "}") {
        if (_tokens[i] == ";") {
            i++; 
            continue;
        }
        else if (_tokens[i] == "root") {
            std::string rootVal;
            _parseRoot(rootVal, i);
            newLocation.setRoot(rootVal);
        }
        else if (_tokens[i] == "autoindex") {
            bool autoindexVal;
            _parseAutoindex(autoindexVal, i);
            newLocation.setAutoindex(autoindexVal);
        }
        else if (_tokens[i] == "index") {
            std::vector<std::string> indexList;
            _parseIndex(indexList, i);
            for (size_t j = 0; j < indexList.size(); ++j) {
                newLocation.addIndex(indexList[j]);
            }
        }
        else {
            throw std::runtime_error("Erro de Sintaxe: Diretiva desconhecida no bloco location: '" + _tokens[i] + "'");
        }
    }
    
    if (i == _tokens.size()) {
        throw std::runtime_error("Erro: Bloco location sem chave de fechamento '}'");
    }
    
    server.addLocation(newLocation);
    
    i++; 
}

std::vector<std::string> ConfigParser::getTokens() const { return _tokens; }
std::vector<ServerConfig> ConfigParser::getServers() const { return _servers; }