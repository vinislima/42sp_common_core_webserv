/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 20:11:00 by vinda-si          #+#    #+#             */
/*   Updated: 2026/09/07 16:18:56 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <sstream>
#include <map>
#include "Request.hpp"
#include "ServerConfig.hpp"

/**
 * @class Response
 * @brief Builds the HTTP response for a parsed Request against a matched ServerConfig.
 *
 * build() is the single entry point: it validates the method, normalizes
 * and resolves the URI against the matched location, then dispatches to
 * GET (static file / autoindex), POST (upload, plain or multipart) or
 * DELETE handling, or to _handleCGI() when the target matches a
 * configured CGI extension. Because file I/O and CGI execution are both
 * asynchronous in this server, build() does not itself write a file or
 * read a CGI's full output — it only opens the fd / spawns the process
 * and leaves the fd in _fileReadFd/_fileWriteFd or _cgiReadFd/_cgiWriteFd
 * for Server.cpp to drive through poll().
 */
class Response {
private:
	int _statusCode;
	std::string _body;
	std::string  _rawResponse;
	std::map<std::string, std::string> _headers;
	std::map<int, std::string> _statusMessages;
	/// @brief Builds a minimal styled HTML page for a status code, used when no custom error_page is configured.
	std::string _getFallbackHTML(int code) const;

	pid_t _cgiPid;
	int _cgiReadFd;
	int _cgiWriteFd;

	int _fileReadFd;  ///< Fd opened for an asynchronous disk read (GET / error page), or -1.
	int _fileWriteFd; ///< Fd opened for an asynchronous disk write (POST upload), or -1.

	/// @brief Populates the status-code -> reason-phrase table used by _generateRawResponse() and error pages.
	void _initStatusMessages();
	/// @brief Serializes _statusCode/_headers/_body into _rawResponse (adds Content-Length and a default Content-Type).
	void _generateRawResponse();
	/// @brief Looks up the MIME type for a file path by its (lower-cased) extension; defaults to text/plain.
	std::string _getContentType(const std::string& path) const;
	/**
	 * @brief Resolves "." and ".." segments out of a request URI.
	 * @param uri The raw request URI (always absolute, starts with "/").
	 * @param out Receives the normalized URI on success.
	 * @return False if the URI is a Path Traversal attempt (a ".." that would go above the root).
	 */
	bool _normalizeUri(const std::string& uri, std::string& out) const;
	/**
	 * @brief Extracts the file part of a multipart/form-data body (RFC 7578).
	 *
	 * Finds the first part whose Content-Disposition carries "filename=",
	 * splitting that part's own headers from its binary content.
	 * @return False if the body has no boundary or no file part.
	 */
	bool _parseMultipart(const std::string& body, const std::string& boundary,
						  std::string& outFilename, std::string& outContent) const;
	/// @brief Builds an error response for `code`, using the server's configured error_page if present, else the fallback HTML.
	void _buildErrorPage(int code, const ServerConfig& config);
	/// @brief Sets up the CGI environment (meta-variables + HTTP_* headers) and forks+execve's the interpreter, wiring its stdin/stdout to pipes.
	void _handleCGI(const std::string& filepath, const std::string& cgiPath, const Request& req, const ServerConfig& config);

public:
	Response();
	~Response();

	void setStatusCode(int code);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	/// @brief Builds the response for `req` against `config` (method/location/CGI/static-file resolution). See class docs for the overall flow.
	void build(Request& req, const ServerConfig& config);

	pid_t getCgiPid() const { return _cgiPid; }
	int getCgiReadFd() const { return _cgiReadFd; }
	int getCgiWriteFd() const { return _cgiWriteFd; }

	int getFileReadFd() const { return _fileReadFd; }
	int getFileWriteFd() const { return _fileWriteFd; }

	/// @return The fully serialized "status-line + headers" (and, for in-memory responses, the body too) ready to send.
	std::string getRawResponse() const;
};

#endif