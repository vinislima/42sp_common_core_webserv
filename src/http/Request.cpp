/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:44 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:09:54 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/Request.hpp"
#include <cctype>

// RFC 7230: HTTP header names are case-insensitive ("Host" == "host" ==
// "HOST"). We normalize to lowercase both when storing and when looking up,
// otherwise a client sending "host:" lowercase (or any other casing) would
// never match the getHeader("Host") calls scattered through the code.
static std::string toLowerCopy(const std::string& s) {
	std::string result = s;
	for (size_t i = 0; i < result.length(); ++i) {
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	}
	return result;
}

Request::Request() : _isComplete(false), _headersParsed(false), _bodyAuthorized(false), _maxBodySize(0), _errorCode(0), _consumedBytes(0), _bodyBytesConsumed(0) {}
Request::Request(const std::string& rawRequest) : _rawRequest(rawRequest), _isComplete(false) {}
Request::Request(const Request& src) { *this = src; }
Request& Request::operator=(const Request& rhs) {
	if (this != &rhs) {
		_rawRequest = rhs._rawRequest;
		_method = rhs._method;
		_uri = rhs._uri;
		_version = rhs._version;
		_headers = rhs._headers;
		_body = rhs._body;
		_isComplete = rhs._isComplete;
		_headersParsed = rhs._headersParsed;
		_bodyAuthorized = rhs._bodyAuthorized;
		_maxBodySize = rhs._maxBodySize;
		_errorCode = rhs._errorCode;
		_consumedBytes = rhs._consumedBytes;
		_bodyBytesConsumed = rhs._bodyBytesConsumed;
	}
	return *this;
}
Request::~Request() {}

void Request::appendToRaw(const std::string& data) {
	_rawRequest += data;
}

void Request::parseHeadersOnly() {
	size_t endOfHeaders = _rawRequest.find("\r\n\r\n");
	if (endOfHeaders == std::string::npos) return;

	size_t endOfFirstLine = _rawRequest.find("\r\n");
	_parseRequestLine(_rawRequest.substr(0, endOfFirstLine));
	_parseHeaders(_rawRequest.substr(endOfFirstLine + 2, endOfHeaders - (endOfFirstLine + 2)));
	_headersParsed = true;
}

void Request::parseBodyOnly() {
	size_t endOfHeaders = _rawRequest.find("\r\n\r\n");
	size_t startOfBody = endOfHeaders + 4;

	// GET/DELETE (or any request without Content-Length/chunked) has no
	// body — must finish here WITHOUT trying to consume whatever comes
	// after the headers, otherwise on Keep-Alive/pipelining this would eat
	// the bytes of the NEXT already-buffered request, treating them as
	// this one's body.
	bool noBodyExpected = (_method == "GET" || _method == "DELETE") ||
			(getHeader("Content-Length").empty() && getHeader("Transfer-Encoding") != "chunked");

	if (noBodyExpected) {
		_isComplete = true;
		_consumedBytes = startOfBody;
		return;
	}

	if (startOfBody >= _rawRequest.length()) {
		return; // body expected, but none of it has arrived yet — wait for more recv()
	}

	try {
		_parseBody(_rawRequest.substr(startOfBody));
	} catch (const std::exception& e) {
		throw;
	}

	if (_isComplete && _errorCode == 0) {
		_consumedBytes = startOfBody + _bodyBytesConsumed;
	}
}

bool Request::isComplete() const { return _isComplete; }

void Request::_parseRequestLine(const std::string& line) {
	std::stringstream ss(line);
	ss >> _method >> _uri >> _version;
	if (_method.empty()) throw std::runtime_error("Invalid Request-Line");
}

void Request::_parseHeaders(const std::string& headersBlock) {
	std::stringstream ss(headersBlock);
	std::string line;
	while (std::getline(ss, line) && line != "\r") {
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = toLowerCopy(line.substr(0, colon));
			std::string val = line.substr(colon + 1);
			size_t start = val.find_first_not_of(" \t");
			if (start != std::string::npos) val = val.substr(start);
			if (!val.empty() && val[val.length() - 1] == '\r') {
				val.erase(val.length() - 1);
			}
			_headers[key] = val;
		}
	}
}

void Request::_parseBody(const std::string& bodyBlock) {
	if (getHeader("Transfer-Encoding") == "chunked") {
		_parseChunkedBody(bodyBlock);
	} else {
		std::string lenStr = getHeader("Content-Length");
		if (!lenStr.empty()) {
			size_t len = std::strtoul(lenStr.c_str(), NULL, 10);

			if (len > _maxBodySize && _maxBodySize > 0) {
				setErrorCode(413);
				return;
			}
			
			if (bodyBlock.length() < len) {
				throw std::runtime_error("Body incompleto");
			}
			_body = bodyBlock.substr(0, len);
			_bodyBytesConsumed = len;
			_isComplete = true;
		} else {
			if (bodyBlock.length() > _maxBodySize && _maxBodySize > 0) {
				setErrorCode(413);
				return;
			}
			_body = bodyBlock;
			_bodyBytesConsumed = bodyBlock.length();
			_isComplete = true;
		}
	}
}
void Request::_parseChunkedBody(const std::string& bodyBlock) {
	size_t pos = 0;
	std::string tempBody = "";

	while (pos < bodyBlock.length()) {
		size_t endOfLine = bodyBlock.find("\r\n", pos);
		if (endOfLine == std::string::npos) throw std::runtime_error("Chunk incompleto");

		std::string hexStr = bodyBlock.substr(pos, endOfLine - pos);
		long chunkSize = std::strtol(hexStr.c_str(), NULL, 16);
		size_t chunkDataStart = endOfLine + 2;

		if (chunkSize == 0) {
			// Terminating chunk ("0\r\n"): still need to consume the final
			// "\r\n" that closes the chunked body (trailers, if any, are
			// ignored). Without this, those bytes would be "lost" in the
			// middle of the buffer and would corrupt Keep-Alive pipelining
			// of the next request.
			size_t terminatorEnd = bodyBlock.find("\r\n", chunkDataStart);
			if (terminatorEnd == std::string::npos) {
				throw std::runtime_error("Chunk incompleto");
			}
			_body = tempBody;
			_bodyBytesConsumed = terminatorEnd + 2;
			_isComplete = true;
			return;
		}

		if (chunkDataStart + static_cast<size_t>(chunkSize) + 2 > bodyBlock.length()) {
			throw std::runtime_error("Chunk incompleto");
		}
		tempBody += bodyBlock.substr(chunkDataStart, chunkSize);

		if (tempBody.length() > _maxBodySize && _maxBodySize > 0) {
			setErrorCode(413);
			return;
		}
		pos = chunkDataStart + chunkSize + 2;
	}

	throw std::runtime_error("Chunk incompleto");
}

std::string Request::getMethod() const { return _method; }
std::string Request::getUri() const { return _uri; }
std::string Request::getVersion() const { return _version; }
std::string Request::getBody() const { return _body; }
std::string Request::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(toLowerCopy(key));
	return (it != _headers.end()) ? it->second : "";
}
std::map<std::string, std::string> Request::getHeaders() const {
	return _headers;
}

std::string Request::extractLeftoverRaw() const {
	if (_consumedBytes >= _rawRequest.length()) return "";
	return _rawRequest.substr(_consumedBytes);
}

bool Request::wantsKeepAlive() const {
	// Grave parsing error (malformed -> 400, body too large -> 413): we
	// can't trust _consumedBytes to find the boundary of the next request,
	// so close the connection to be safe.
	if (_errorCode != 0) return false;

	std::string connection = toLowerCopy(getHeader("Connection"));

	if (_version == "HTTP/1.1") {
		return connection != "close";
	}
	if (_version == "HTTP/1.0") {
		return connection == "keep-alive";
	}
	return false; // unknown version: safer to close
}