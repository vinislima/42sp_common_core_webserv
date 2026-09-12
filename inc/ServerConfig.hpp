/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:28 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/12 18:57:51 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>

#include "LocationConfig.hpp"

/**
 * @class ServerConfig
 * @brief Configuration for a single "server { ... }" block in the .conf file.
 *
 * Holds everything declared directly inside a server block (listen
 * address/port, server_name aliases, error_page map, root, autoindex,
 * index files, client_max_body_size) plus the list of nested
 * LocationConfig route overrides. Server.cpp picks the right ServerConfig
 * for an incoming connection by matching listen port + Host header, then
 * resolves per-request overrides through getBestMatchLocation().
 */
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

	/**
	 * @brief Finds the location whose path is the longest prefix match of `uri`.
	 *
	 * Same rule nginx uses for prefix locations: among every LocationConfig
	 * whose path is a prefix of `uri`, the one with the longest path wins.
	 * Used both by Response::build() (to resolve root/index/CGI/redirects
	 * for the final response) and by Server::_parseClientRequest() (to
	 * resolve a per-location client_max_body_size before the request body
	 * has even fully arrived).
	 * @param uri The request URI (query string already stripped), to match against each location's path.
	 * @return The best-matching LocationConfig, or NULL if no location's path prefixes `uri`.
	 */
	const LocationConfig* getBestMatchLocation(const std::string& uri) const;
};

#endif
