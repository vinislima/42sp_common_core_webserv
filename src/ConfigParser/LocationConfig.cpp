#include "../../inc/LocationConfig.hpp"

LocationConfig::LocationConfig() : _path(""), _root(""), _autoindex(false) {}

LocationConfig::LocationConfig(const std::string& path) : _path(path), _root(""), _autoindex(false) {}

LocationConfig::LocationConfig(const LocationConfig& src) {
    *this = src;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& rhs) {
    if (this != &rhs) {
        this->_path = rhs._path;
        this->_root = rhs._root;
        this->_autoindex = rhs._autoindex;
        this->_index = rhs._index;
    }
    return *this;
}

LocationConfig::~LocationConfig() {}

void LocationConfig::setPath(const std::string& path) { this->_path = path; }
void LocationConfig::setRoot(const std::string& root) { this->_root = root; }
void LocationConfig::setAutoindex(bool autoindex) { this->_autoindex = autoindex; }
void LocationConfig::addIndex(const std::string& index) { this->_index.push_back(index); }

std::string LocationConfig::getPath() const { return this->_path; }
std::string LocationConfig::getRoot() const { return this->_root; }
bool LocationConfig::getAutoindex() const { return this->_autoindex; }
std::vector<std::string> LocationConfig::getIndex() const { return this->_index; }