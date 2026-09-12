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
#include <cstdlib>

#include "ServerConfig.hpp"

/**
 * @class ConfigParser
 * @brief Reads an nginx-style .conf file and builds the server configuration tree.
 *
 * Parsing happens in two passes: _tokenize() splits the raw file content
 * into a flat list of tokens (words, and the standalone "{", "}", ";"
 * punctuation), stripping "#" comments; _buildTree() then walks that token
 * list and turns each top-level "server { ... }" block into a ServerConfig,
 * with nested "location { ... }" blocks becoming that server's
 * LocationConfig list. Each recognized directive (listen, server_name,
 * root, ...) has its own _parseXxx() helper that consumes the directive's
 * tokens (advancing the shared index `i`) and validates its syntax,
 * throwing std::runtime_error on anything malformed.
 */
class ConfigParser {
private:
	std::string					_configFile; ///< Path to the .conf file to parse.
	std::vector<std::string>	_tokens;     ///< Flat token stream produced by _tokenize().
	std::vector<ServerConfig>	_servers;    ///< Parsed server{} blocks, filled in by _buildTree().

	/// @brief Splits raw file content into tokens, discarding whitespace and "#" comments.
	void	_tokenize(const std::string& content);

	/// @brief Walks the token stream and builds one ServerConfig per top-level "server{}" block.
	void	_buildTree();
	/// @brief Parses a "listen [host][:port];" directive into the given server.
	void	_parseListen(ServerConfig& server, size_t& i);
	/// @brief Parses a "server_name name1 name2 ...;" directive into the given server.
	void	_parseServerName(ServerConfig& server, size_t& i);
	/// @brief Parses one or more "error_page code... uri;" directives into the given server.
	void	_parseErrorPage(ServerConfig& server, size_t& i);
	/// @brief Parses a "client_max_body_size N[k|m|g];" directive, resolving the unit suffix into bytes.
	void	_parseClientMaxBodySize(size_t& outSize, size_t& i);

	/// @brief Parses a "root path;" directive.
	void	_parseRoot(std::string& outRoot, size_t& i);
	/// @brief Parses an "autoindex on|off;" directive.
	void	_parseAutoindex(bool& outAutoindex, size_t& i);
	/// @brief Parses an "index file1 file2 ...;" directive.
	void	_parseIndex(std::vector<std::string>& outIndexList, size_t& i);
	/// @brief Parses a nested "location path { ... }" block into a new LocationConfig, added to the server.
	void	_parseLocation(ServerConfig& server, size_t& i);
	/// @brief Parses a "return code url;" redirect directive into the given location.
	void	_parseReturn(LocationConfig& location, size_t& i);
	/// @brief Parses an "upload_store path;" directive into the given location.
	void	_parseUploadStore(std::string& outPath, size_t& i);
	/// @brief Parses an "allow_methods GET POST ...;" directive into the given location.
	void	_parseAllowMethods(LocationConfig& location, size_t& i);

public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& src);
	ConfigParser& operator=(const ConfigParser& rhs);
	~ConfigParser();

	/// @brief Reads _configFile from disk and parses it into _servers. Throws std::runtime_error on any syntax error.
	void						parse();
	/// @return The flat token stream produced by the last parse().
	std::vector<std::string>	getTokens() const;
	/// @return The list of ServerConfig objects built from the parsed file.
	std::vector<ServerConfig>	getServers() const;
};

#endif
