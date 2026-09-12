/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:25 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/12 19:00:36 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <poll.h>
#include <cerrno>
#include <map>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

#include "ServerConfig.hpp"
#include "Request.hpp"
#include "Client.hpp"

/**
 * @class Server
 * @brief Owns every listening socket and client connection, driven by a single poll() loop.
 *
 * Construction takes the list of ServerConfig (one per "server{}" block,
 * possibly several sharing the same host:port as virtual hosts). start()
 * opens/binds/listens on every distinct host:port (_setupSockets()) and
 * then runs _runEventLoop() forever, which is the only poll() call in the
 * program: each iteration dispatches ready fds to one of three groups —
 * CGI pipes, asynchronous file I/O fds, or plain client sockets — handling
 * new connections, incoming request bytes, outgoing response bytes, and
 * both the idle-connection and CGI-runtime timeouts.
 */
class Server {
private:
	std::vector<ServerConfig>	_configs;	///< One entry per "server{}" block from the .conf file.
	std::vector<int>			_listenSockets;
	std::vector<struct pollfd>	_pollFds;	///< Every fd currently watched by poll(): listen sockets, client sockets, CGI pipes, file I/O fds.
	std::map<int, Client>		_clients;	///< Per-connection state, keyed by client socket fd.
	std::map<int, int> _cgiToClient;		///< Maps a CGI pipe fd back to the client fd it belongs to.
	std::map<int, int> _fileToClient;		///< Maps an asynchronous file I/O fd back to the client fd it belongs to.
	std::map<int, int> _listenFdToPort;		///< Maps a listening socket fd to the port it was bound on.
	std::map<int, int> _clientToListenFd;	///< Maps a client fd to the listening socket that accepted it (used to resolve the port for virtual-host matching).

	/// @brief Creates, binds and listens on one socket per distinct host:port across all configs, adding each to _pollFds.
	void	_setupSockets();
	bool	_isListenSocket(int fd);
	/// @brief Accepts a pending connection on `listenFd`, registers the new client fd with poll() and _clients.
	void	_acceptNewConnection(int listenFd);
	/// @brief Reads available bytes from `clientFd` into its Request and triggers parsing; returns false if the peer closed the connection.
	bool	_handleClientRead(int clientFd);
	/// @brief Parses whatever has been buffered so far for `clientFd` (headers, then body), resolving the matched config/location and enforcing the header-size and body-size limits.
	void	_parseClientRequest(int clientFd);
	/// @brief Builds the response (if not already built), or continues sending a response already in flight; returns false when the connection should close.
	bool	_handleClientWrite(int clientFd);
	/// @brief The single poll() loop: dispatches every ready fd to CGI, file-I/O or client handling, then runs the timeout checks.
	void	_runEventLoop();
	/// @brief Closes any client connection idle for longer than the idle timeout (skips connections with a CGI or file I/O in flight).
	void	_checkTimeouts();
	/// @brief Kills and reaps any CGI child that has been running longer than the CGI timeout, responding 504 to its client.
	void	_checkCgiTimeouts();
	/// @brief Closes a client socket and removes all bookkeeping (Client, pollfd entry, fd maps) for it.
	void	_closeClient(int clientFd, size_t pollIdx);
	/// @brief Removes a single fd's entry from _pollFds, if present.
	void	_erasePollFd(int fd);
	/**
	 * @brief Resolves the ServerConfig for a client, matching the listening port first and then the Host header (virtual hosts).
	 * @return The server whose server_name matches `hostHeader` on that port, or the first server{} listening on that port if none matches, or the very first config as a last resort.
	 */
	const ServerConfig*	_matchConfig(int clientFd, const std::string& hostHeader) const;


public:
	Server();
	Server(const std::vector<ServerConfig>& configs);
	Server(const Server& src);
	Server& operator=(const Server& rhs);
	~Server();

	/// @brief Sets up all listening sockets and runs the event loop; does not return under normal operation.
	void	start();
};

#endif