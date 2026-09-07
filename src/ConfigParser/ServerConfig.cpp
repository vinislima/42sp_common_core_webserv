/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:38 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:39 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/ServerConfig.hpp"


ServerConfig::ServerConfig() : _host("0.0.0.0"), _port(80), _clientMaxBodySize(1048576), _root(""), _autoindex(false) {}

ServerConfig::ServerConfig(const ServerConfig& src) {
	*this = src;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& rhs) {
	if (this != &rhs) {
		this->_host = rhs._host;
		this->_port = rhs._port;
		this->_serverNames = rhs._serverNames;
		this->_errorPages = rhs._errorPages;
		this->_clientMaxBodySize = rhs._clientMaxBodySize;
		this->_root = rhs._root;
		this->_autoindex = rhs._autoindex;
		this->_index = rhs._index;
		this->_locations = rhs._locations;
	}
	return *this;
}

ServerConfig::~ServerConfig() {}

void ServerConfig::setHost(const std::string& host) {
	this->_host = host;
}

void ServerConfig::setPort(int port) {
	this->_port = port;
}

void ServerConfig::addServerName(const std::string& name) {
	this->_serverNames.push_back(name);
}

void ServerConfig::addErrorPage(int code, const std::string& uri) {
	this->_errorPages[code] = uri;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
	this->_clientMaxBodySize = size;
}

void ServerConfig::setRoot(const std::string& root) {
	this->_root = root;
}

void ServerConfig::setAutoindex(bool autoindex) {
	this->_autoindex = autoindex;
}

void ServerConfig::addIndex(const std::string& index) {
	this->_index.push_back(index);
}

void ServerConfig::addLocation(const LocationConfig& location) {
	this->_locations.push_back(location);
}

std::string ServerConfig::getHost() const {
	return this->_host;
}

int ServerConfig::getPort() const {
	return this->_port;
}

std::vector<std::string> ServerConfig::getServerNames() const {
	return this->_serverNames;
}

std::map<int, std::string> ServerConfig::getErrorPages() const {
	return this->_errorPages;
}

size_t ServerConfig::getClientMaxBodySize() const {
	return this->_clientMaxBodySize;
}

std::string ServerConfig::getRoot() const {
	return this->_root;
}

bool ServerConfig::getAutoindex() const {
	return this->_autoindex;
}

std::vector<std::string> ServerConfig::getIndex() const {
	return this->_index;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
	return this->_locations;
}

const LocationConfig* ServerConfig::getBestMatchLocation(const std::string& uri) const {
	const LocationConfig* bestMatch = NULL;
	size_t longestMatchLength = 0;

	for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it) {
		std::string locPath = it->getPath();
		if (uri.find(locPath) == 0) {
			if (locPath.length() > longestMatchLength) {
				longestMatchLength = locPath.length();
				bestMatch = &(*it);
			}
		}
	}
	return bestMatch;
}