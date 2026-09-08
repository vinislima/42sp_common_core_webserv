/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:14 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/07 16:17:03 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib> // For atoi()

#include "ServerConfig.hpp" // <-- Include the new class

class ConfigParser {
private:
	std::string					_configFile;
	std::vector<std::string>	_tokens;
	std::vector<ServerConfig>	_servers; // <-- Our list of ready servers

	void	_tokenize(const std::string& content);

	// New architecture methods for the current issue:
	void	_buildTree();
	void	_parseListen(ServerConfig& server, size_t& i);
	void	_parseServerName(ServerConfig& server, size_t& i);
	void	_parseErrorPage(ServerConfig& server, size_t& i);
	void	_parseClientMaxBodySize(size_t& outSize, size_t& i);

	void	_parseRoot(std::string& outRoot, size_t& i);
	void	_parseAutoindex(bool& outAutoindex, size_t& i);
	void	_parseIndex(std::vector<std::string>& outIndexList, size_t& i);
	void	_parseLocation(ServerConfig& server, size_t& i);
	void	_parseReturn(LocationConfig& location, size_t& i);
	void	_parseUploadStore(std::string& outPath, size_t& i);
	void	_parseAllowMethods(LocationConfig& location, size_t& i);

public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& src);
	ConfigParser& operator=(const ConfigParser& rhs);
	~ConfigParser();

	void						parse();
	std::vector<std::string>	getTokens() const;
	std::vector<ServerConfig>	getServers() const; // <-- To get the ready servers
};

#endif
