#include "../../inc/Request.hpp"

Request::Request() : _isComplete(false) {}
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
	// size_t startOfBody = endOfHeaders + 4;
	// if (startOfBody < _rawRequest.length()) {
	// 	try {
	// 		_parseBody(_rawRequest.substr(startOfBody));
	// 	} catch (const std::exception& e) {
	// 		std::cout << "[DEBUG] Erro no Parse do Body: " << e.what() << "\n";
	// 		throw; 
	// 	}
	// }
	// _isComplete = true;
}

void Request::parseBodyOnly() {
	size_t endOfHeaders = _rawRequest.find("\r\n\r\n");
	size_t startOfBody = endOfHeaders + 4;

	if (startOfBody < _rawRequest.length()) {
		try {
			_parseBody(_rawRequest.substr(startOfBody));
		} catch (const std::exception& e) {
			throw;
		}
	} else if (_method == "GET" || _method == "DELETE" || 
			(getHeader("Content-Length").empty() && getHeader("Transfer-Enconding") != "chunked")) {
		_isComplete = true;
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
			std::string key = line.substr(0, colon);
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
			_isComplete = true;
		} else {
			if (bodyBlock.length() > _maxBodySize && _maxBodySize > 0) {
				setErrorCode(413);
				return;
			}
			_body = bodyBlock;
			_isComplete = true;
		}
	}
}
void Request::_parseChunkedBody(const std::string& bodyBlock) {
	size_t pos = 0;
	std::string tempBody = "";

	while (pos < bodyBlock.length()) {
		size_t endOfLine = bodyBlock.find("\r\n", pos);
		if (endOfLine == std::string::npos) break; 
		
		std::string hexStr = bodyBlock.substr(pos, endOfLine - pos);
		long chunkSize = std::strtol(hexStr.c_str(), NULL, 16);
		
		if (chunkSize == 0) {
			_body = tempBody;
			_isComplete = true;
			break;
		}
		pos = endOfLine + 2;

		if (pos + chunkSize > bodyBlock.length()) {
			throw std::runtime_error("Chunk incompleto");
		}
		tempBody += bodyBlock.substr(pos, chunkSize);

		if (tempBody.length() > _maxBodySize && _maxBodySize > 0) {
			setErrorCode(413);
			return;
		}
		pos += chunkSize + 2;
	}
}

std::string Request::getMethod() const { return _method; }
std::string Request::getUri() const { return _uri; }
std::string Request::getVersion() const { return _version; }
std::string Request::getBody() const { return _body; }
std::string Request::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	return (it != _headers.end()) ? it->second : "";
}
std::map<std::string, std::string> Request::getHeaders() const {
	return _headers;
}