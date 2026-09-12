/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:21 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:22 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <cstdlib>

/**
 * @class Request
 * @brief Incrementally parses one HTTP request out of a client's raw byte stream.
 *
 * Bytes arrive in arbitrary chunks over several recv() calls, so parsing is
 * split into two independently-retriable steps: parseHeadersOnly() (waits
 * for the "\r\n\r\n" that ends the header block) and parseBodyOnly() (waits
 * for the body to be fully available, per Content-Length or chunked
 * Transfer-Encoding). isComplete() reports when the request — including any
 * body — is fully parsed and ready to be handed to Response::build().
 *
 * Because a client on Keep-Alive can pipeline a second request right after
 * the first in the same buffer, this class also tracks exactly how many
 * bytes of _rawRequest belong to the current request (_consumedBytes) so
 * extractLeftoverRaw() can hand any remaining bytes to the next Request.
 */
class Request {
private:
	std::string _rawRequest;
	std::string _method;
	std::string _uri;
	std::string _version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	bool	_isComplete;
	bool	_headersParsed;
	bool	_bodyAuthorized;
	size_t	_maxBodySize;
	int		_errorCode;
	size_t	_consumedBytes;     ///< Bytes of _rawRequest belonging to this request (headers + body).
	size_t	_bodyBytesConsumed; ///< Subset of _consumedBytes that is just the body, set by _parseBody()/_parseChunkedBody().

	/// @brief Splits the request-line into method/URI/version, validating the HTTP version and token count.
	void _parseRequestLine(const std::string& line);
	/// @brief Parses "Key: value" header lines, lower-casing keys for case-insensitive lookup.
	void _parseHeaders(const std::string& headersBlock);
	/// @brief Reads the body per Content-Length (or delegates to _parseChunkedBody() for chunked encoding).
	void _parseBody(const std::string& bodyBlock);
	/// @brief Decodes a chunked Transfer-Encoding body, concatenating chunks until the terminating 0-length chunk.
	void _parseChunkedBody(const std::string& bodyBlock);

public:
	Request();
	Request(const std::string& rawRequest);
	Request(const Request& src);
	Request& operator=(const Request& rhs);
	~Request();

	/// @brief Appends newly-received bytes to the internal raw buffer.
	void	appendToRaw(const std::string& data);
	void	setBody(const std::string& body) { _body = body; }
	void	setBodyAuthorized(bool auth) { _bodyAuthorized = auth; }
	void	setMaxBodySize(size_t size) { _maxBodySize = size; }
	/// @brief Attempts to parse the request-line + headers once "\r\n\r\n" has fully arrived; sets an error code on a malformed or missing mandatory Host header.
	void	parseHeadersOnly();
	/// @brief Attempts to parse the body once it is fully available; no-op (waits for more data) if it hasn't all arrived yet.
	void	parseBodyOnly();
	/// @return True once the request (headers, and body if any) is fully parsed, or an error code was set.
	bool	isComplete() const;
	bool	areHeadersParsed() const { return _headersParsed; }
	/// @return Current length of the raw buffer; used by Server::_parseClientRequest() to enforce the header-size limit.
	size_t	getRawLength() const { return _rawRequest.length(); }
	bool	isBodyAuthorized() const { return _bodyAuthorized; }
	size_t	getMaxBodySize() const { return _maxBodySize; }
	int		getErrorCode() const { return _errorCode; }
	/// @brief Marks the request as failed with the given HTTP status code and immediately complete (no further parsing).
	void	setErrorCode(int code) {
		_errorCode = code;
		_isComplete = true;
	}

	std::string getMethod() const;
	std::string getUri() const;
	std::string getVersion() const;
	/// @return The parsed request body, by reference so repeated writers (e.g. streaming it to a CGI or a file across several poll() events) don't copy it on every call.
	const std::string& getBody() const;
	/// @brief Case-insensitive header lookup.
	/// @return The header's value, or "" if not present.
	std::string getHeader(const std::string& key) const;
	std::map<std::string, std::string> getHeaders() const;

	/// @return Bytes left in the raw buffer after this request — belongs to a pipelined next request, if any.
	std::string extractLeftoverRaw() const;
	/// @return True if the connection should stay open for another request (HTTP/1.1 without "Connection: close", or HTTP/1.0 with "Connection: keep-alive"); always false after a parsing error.
	bool wantsKeepAlive() const;
};

#endif