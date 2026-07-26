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
} // <-- Chave de fechamento que estava faltando!

void ConfigParser::_buildTree() {
    for (size_t i = 0; i < _tokens.size(); ++i) {
        if (_tokens[i] == "server") {
            i++; // Pula a palavra "server"
            if (i >= _tokens.size() || _tokens[i] != "{") {
                throw std::runtime_error("Erro: Bloco server sem chave de abertura '{'");
            }
            i++; // Pula a chave "{"

            ServerConfig newServer;

            // Fica num loop preenchendo a caixa até encontrar o "}" do server
            while (i < _tokens.size() && _tokens[i] != "}") {
                if (_tokens[i] == "listen") {
                    _parseListen(newServer, i);
                }
                else if (_tokens[i] == "server_name") {
                    _parseServerName(newServer, i);
                }
                else {
                    // Por enquanto ignoramos outras diretivas (root, error_page)
                    i++;
                }
            }
            
            if (i == _tokens.size()) {
                throw std::runtime_error("Erro: Bloco server sem chave de fechamento '}'");
            }

            // Guarda o servidor pronto na lista
            _servers.push_back(newServer);
        }
    }
}

void ConfigParser::_parseListen(ServerConfig& server, size_t& i) {
    i++; // Pula a palavra "listen"
    if (i >= _tokens.size() || _tokens[i] == ";") {
         throw std::runtime_error("Erro: Diretiva listen vazia");
    }

    std::string value = _tokens[i];
    size_t colonPos = value.find(':');

    if (colonPos != std::string::npos) {
        // Tem ':' na string (ex: "127.0.0.1:8080")
        std::string host = value.substr(0, colonPos);
        std::string portStr = value.substr(colonPos + 1);
        
        server.setHost(host);
        server.setPort(std::atoi(portStr.c_str()));
    } else {
        // Não tem ':'.
        if (std::isdigit(value[0])) {
            server.setPort(std::atoi(value.c_str()));
        } else {
            server.setHost(value);
        }
    }

    i++; // Pula o valor
    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva listen sem ponto e vírgula ';'");
    }
}

void ConfigParser::_parseServerName(ServerConfig& server, size_t& i) {
    i++; // Pula a palavra "server_name"
    
    // Pode haver múltiplos nomes (ex: server_name site.com www.site.com;)
    while (i < _tokens.size() && _tokens[i] != ";") {
        server.addServerName(_tokens[i]);
        i++;
    }

    if (i >= _tokens.size() || _tokens[i] != ";") {
         throw std::runtime_error("Erro: Diretiva server_name sem ponto e vírgula ';'");
    }
}

// =============================================================================
// Getters
// =============================================================================

std::vector<std::string> ConfigParser::getTokens() const { return _tokens; }
std::vector<ServerConfig> ConfigParser::getServers() const { return _servers; }