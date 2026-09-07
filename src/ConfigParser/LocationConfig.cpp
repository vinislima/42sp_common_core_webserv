/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:36 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:37 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/LocationConfig.hpp"

LocationConfig::LocationConfig() : _path(""), _root(""), _autoindex(AUTOINDEX_INHERIT), _redirectCode(0), _redirectUrl(""), _clientMaxBodySize(0), _hasClientMaxBodySize(false), _uploadStore("") {}

LocationConfig::LocationConfig(const std::string& path) : _path(path), _root(""), _autoindex(AUTOINDEX_INHERIT), _redirectCode(0), _redirectUrl(""), _clientMaxBodySize(0), _hasClientMaxBodySize(false), _uploadStore("") {}

LocationConfig::LocationConfig(const LocationConfig& src) {
	*this = src;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& rhs) {
    if (this != &rhs) {
        this->_path = rhs._path;
        this->_root = rhs._root;
        this->_autoindex = rhs._autoindex;
        this->_index = rhs._index;
        this->_cgiExt = rhs._cgiExt;
        this->_cgiPath = rhs._cgiPath;
		this->_redirectCode = rhs._redirectCode;
		this->_redirectUrl = rhs._redirectUrl;
		this->_clientMaxBodySize = rhs._clientMaxBodySize;
		this->_hasClientMaxBodySize = rhs._hasClientMaxBodySize;
	    this->_uploadStore = rhs._uploadStore;
		this->_allowMethods = rhs._allowMethods;
	}
	return *this;
}

LocationConfig::~LocationConfig() {}

void LocationConfig::setPath(const std::string& path) { this->_path = path; }
void LocationConfig::setRoot(const std::string& root) { this->_root = root; }
void LocationConfig::setAutoindex(bool autoindex) { this->_autoindex = autoindex ? AUTOINDEX_ON : AUTOINDEX_OFF; }
void LocationConfig::addIndex(const std::string& index) { this->_index.push_back(index); }

void LocationConfig::setRedirect(int code, const std::string& url) {
	this->_redirectCode = code;
	this->_redirectUrl = url;
}

void LocationConfig::setClientMaxBodySize(size_t size) {
	this->_clientMaxBodySize = size;
	this->_hasClientMaxBodySize = true;
}

void LocationConfig::setUploadStore(const std::string& path) {
	this->_uploadStore = path;
}

void LocationConfig::addAllowMethod(const std::string& method) { this->_allowMethods.push_back(method); }


int LocationConfig::getRedirectCode() const {
	return this->_redirectCode;
}

std::string LocationConfig::getRedirectUrl() const {
	return this->_redirectUrl;
}

std::string LocationConfig::getPath() const { return this->_path; }
std::string LocationConfig::getRoot() const { return this->_root; }
LocationConfig::AutoindexState LocationConfig::getAutoindexState() const { return this->_autoindex; }
std::vector<std::string> LocationConfig::getIndex() const { return this->_index; }
size_t LocationConfig::getClientMaxBodySize() const { return this->_clientMaxBodySize; }
bool LocationConfig::hasClientMaxBodySize() const { return this->_hasClientMaxBodySize; }

void LocationConfig::setCgiExt(const std::string& ext) { this->_cgiExt = ext; }
void LocationConfig::setCgiPath(const std::string& path) { this->_cgiPath = path; }
std::string LocationConfig::getCgiExt() const { return this->_cgiExt; }
std::string LocationConfig::getCgiPath() const { return this->_cgiPath; }
std::string LocationConfig::getUploadStore() const { return this->_uploadStore; }

std::vector<std::string> LocationConfig::getAllowMethods() const { return this->_allowMethods; }

bool LocationConfig::isMethodAllowed(const std::string& method) const {
    // If the directive isn't defined in the .conf, allow everything by default
    if (this->_allowMethods.empty()) return true; 
    
    for (size_t i = 0; i < this->_allowMethods.size(); ++i) {
        if (this->_allowMethods[i] == method) return true;
    }
    return false;
}