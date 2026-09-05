#include "../../inc/LocationConfig.hpp"

LocationConfig::LocationConfig() : _path(""), _root(""), _autoindex(false), _redirectCode(0), _redirectUrl(""), _uploadStore("") {}

LocationConfig::LocationConfig(const std::string& path) : _path(path), _root(""), _autoindex(false), _redirectCode(0), _redirectUrl(""), _uploadStore("") {}

LocationConfig::LocationConfig(const LocationConfig& src) {
	*this = src;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& rhs) {
	if (this != &rhs) {
		this->_path = rhs._path;
		this->_root = rhs._root;
		this->_autoindex = rhs._autoindex;
		this->_index = rhs._index;
		this->_redirectCode = rhs._redirectCode;
		this->_redirectUrl = rhs._redirectUrl;
		this->_uploadStore = rhs._uploadStore;
	}
	return *this;
}

LocationConfig::~LocationConfig() {}

void LocationConfig::setPath(const std::string& path) { this->_path = path; }
void LocationConfig::setRoot(const std::string& root) { this->_root = root; }
void LocationConfig::setAutoindex(bool autoindex) { this->_autoindex = autoindex; }
void LocationConfig::addIndex(const std::string& index) { this->_index.push_back(index); }

void LocationConfig::setRedirect(int code, const std::string& url) {
	this->_redirectCode = code;
	this->_redirectUrl = url;
}

void LocationConfig::setUploadStore(const std::string& path) {
	this->_uploadStore = path;
}

int LocationConfig::getRedirectCode() const {
	return this->_redirectCode;
}

std::string LocationConfig::getRedirectUrl() const {
	return this->_redirectUrl;
}

std::string LocationConfig::getPath() const { return this->_path; }
std::string LocationConfig::getRoot() const { return this->_root; }
bool LocationConfig::getAutoindex() const { return this->_autoindex; }
std::vector<std::string> LocationConfig::getIndex() const { return this->_index; }
std::string LocationConfig::getUploadStore() const { return this->_uploadStore; }
