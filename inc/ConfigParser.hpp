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

class ConfigParser {
private:
	std::string				 _configFile;
	std::vector<std::string>	_tokens;
	std::vector<ServerConfig>   _servers;

	void	_tokenize(const std::string& content);

	void	_buildTree();
	void	_parseListen(ServerConfig& server, size_t& i);
	void	_parseServerName(ServerConfig& server, size_t& i);
	void	_parseErrorPage(ServerConfig& server, size_t& i);
	void	_parseClientMaxBodySize(ServerConfig& server, size_t& i);
	void	_parseRoot(std::string& outRoot, size_t& i);
	void	_parseAutoindex(bool& outAutoindex, size_t& i);
	void	_parseIndex(std::vector<std::string>& outIndexList, size_t& i);
	void	_parseLocation(ServerConfig& server, size_t& i);
	void	_parseReturn(LocationConfig& location, size_t& i);
	void	_parseUploadStore(std::string& outPath, size_t& i);

public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& src);
	ConfigParser& operator=(const ConfigParser& rhs);
	~ConfigParser();

	void						parse();
	std::vector<std::string>	getTokens() const;
	std::vector<ServerConfig>   getServers() const; 
};

#endif
