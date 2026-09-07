/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:28 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:29 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>

#include "LocationConfig.hpp" 

class ServerConfig {
private:
	std::string					_host;
	int							_port;
	std::vector<std::string>	_serverNames;
	std::map<int, std::string>	_errorPages;
	size_t						_clientMaxBodySize;

	std::string					_root;
	bool						_autoindex;
	std::vector<std::string>	_index;
	std::vector<LocationConfig> _locations; 

public:
	ServerConfig();
	ServerConfig(const ServerConfig& src);
	ServerConfig& operator=(const ServerConfig& rhs);
	~ServerConfig();

	void	setHost(const std::string& host);
	void	setPort(int port);
	void	addServerName(const std::string& name);
	void	addErrorPage(int code, const std::string& uri);
	void	setClientMaxBodySize(size_t size);
		
	void	setRoot(const std::string& root);
	void	setAutoindex(bool autoindex);
	void	addIndex(const std::string& index);
	void	addLocation(const LocationConfig& location);

	std::string					getHost() const;
	int							getPort() const;
	std::vector<std::string>	getServerNames() const;
	std::map<int, std::string>	getErrorPages() const;
	size_t						getClientMaxBodySize() const;

	std::string					getRoot() const;
	bool						getAutoindex() const;
	std::vector<std::string>	getIndex() const;
	const std::vector<LocationConfig>& getLocations() const;

	// Longest-prefix match, same as nginx. Moved here (from Response, where
	// it originally lived) so Server.cpp can also resolve the location for a
	// URI — needed to apply a per-location client_max_body_size before the
	// body even starts arriving, not just once Response::build() runs.
	const LocationConfig* getBestMatchLocation(const std::string& uri) const;
};

#endif
