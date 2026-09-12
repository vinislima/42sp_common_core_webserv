/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 09:41:21 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/12 19:00:40 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

/**
 * @class Client
 * @brief Per-connection state tracked by the Server's single poll() loop.
 *
 * One Client exists for each accepted TCP connection and lives in
 * Server::_clients, keyed by the client socket fd. It is a plain data
 * holder (no real behavior beyond bookkeeping): the request being parsed,
 * the outgoing response buffer, and the extra fds/state needed while a
 * CGI child process or an asynchronous disk read/write is in flight for
 * this connection. On HTTP Keep-Alive, the object is reset to a fresh
 * Client() between requests on the same socket.
 */
class Client {
public:
	Request req;				///< Request currently being parsed/served on this connection.
	std::string responseBuffer;	///< Bytes still to be sent back to the client.
	size_t bytesSent;			///< How much of responseBuffer has already been sent.
	bool isReadyToSend;			///< True once responseBuffer holds a full response ready to flush.
	time_t lastActivity;		///< Timestamp of the last read/write, used by the idle-connection timeout.

	bool isCgi;				///< True while a CGI child process is running for this request.
	pid_t cgiPid;			///< PID of the running CGI child.
	int cgiReadFd;			///< Read end of the CGI's stdout pipe (CGI output).
	int cgiWriteFd;			///< Write end of the CGI's stdin pipe (request body forwarded to the CGI).
	std::string cgiOutput;	///< Raw bytes collected so far from the CGI's stdout.
	size_t cgiBytesWritten;	///< Bytes of the request body already written to the CGI's stdin.
	time_t cgiStart;		///< When the CGI was launched, used by the CGI-specific timeout.

	bool isFile;			///< True while an asynchronous disk read/write is in flight for this request.
	int fileReadFd;			///< Fd being read from disk (GET / error page).
	int fileWriteFd;		///< Fd being written to disk (POST upload).
	size_t fileBytesWritten;	///< Bytes of the request body already written to fileWriteFd.

	Client() : bytesSent(0), isReadyToSend(false), isCgi(false), cgiPid(-1),
				cgiReadFd(-1), cgiWriteFd(-1), cgiBytesWritten(0), cgiStart(0),
				isFile(false), fileReadFd(-1), fileWriteFd(-1), fileBytesWritten(0) {
		lastActivity = time(NULL);
	}

	~Client() {}

	/// @brief Refreshes lastActivity to now; call on every read/write on this connection.
	void updateActivity() {
		lastActivity = time(NULL);
	}
};

#endif