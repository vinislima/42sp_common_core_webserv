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
	size_t	_consumedBytes;    // bytes of _rawRequest used by THIS request (for Keep-Alive/pipelining)
	size_t	_bodyBytesConsumed; // same, but just the body part (filled in by _parseBody/_parseChunkedBody)
	void _parseRequestLine(const std::string& line);
	void _parseHeaders(const std::string& headersBlock);
	void _parseBody(const std::string& bodyBlock);
	void _parseChunkedBody(const std::string& bodyBlock);

public:
	Request();
	Request(const std::string& rawRequest);
	Request(const Request& src);
	Request& operator=(const Request& rhs);
	~Request();

	void	appendToRaw(const std::string& data);
	void	setBody(const std::string& body) { _body = body; }
	void	setBodyAuthorized(bool auth) { _bodyAuthorized = auth; }
	void	setMaxBodySize(size_t size) { _maxBodySize = size; }
	void	parseHeadersOnly();
	void	parseBodyOnly();
	bool	isComplete() const;
	bool	areHeadersParsed() const { return _headersParsed; }
	bool	isBodyAuthorized() const { return _bodyAuthorized; }
	size_t	getMaxBodySize() const { return _maxBodySize; }
	int		getErrorCode() const { return _errorCode; }
	void	setErrorCode(int code) {
		_errorCode = code;
		_isComplete = true;
	}
	
	std::string getMethod() const;
	std::string getUri() const;
	std::string getVersion() const;
	std::string getBody() const;
	std::string getHeader(const std::string& key) const;
	std::map<std::string, std::string> getHeaders() const;

	// Keep-Alive / pipelining: bytes of _rawRequest left over after this
	// request (already belong to the next one, if the client already sent
	// them) and whether this connection should stay open for the next request.
	std::string extractLeftoverRaw() const;
	bool wantsKeepAlive() const;
};

#endif