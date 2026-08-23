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
	}
	return *this;
}
Request::~Request() {}

void Request::appendToRaw(const std::string& data) {
	_rawRequest += data;
}

void Request::parse() {
	size_t endOfHeaders = _rawRequest.find("\r\n\r\n");
	if (endOfHeaders == std::string::npos) return;

	size_t endOfFirstLine = _rawRequest.find("\r\n");
	_parseRequestLine(_rawRequest.substr(0, endOfFirstLine));

	_parseHeaders(_rawRequest.substr(endOfFirstLine + 2, endOfHeaders - (endOfFirstLine + 2)));

	size_t startOfBody = endOfHeaders + 4;
	if (startOfBody < _rawRequest.length()) {
		try {
			_parseBody(_rawRequest.substr(startOfBody));
		} catch (const std::exception& e) {
			std::cout << "[DEBUG] Erro no Parse do Body: " << e.what() << "\n";
			throw; 
		}
	}

	_isComplete = true; 
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
		std::string len = getHeader("Content-Length");
		if (!len.empty()) _body = bodyBlock.substr(0, std::atoi(len.c_str()));
		else _body = bodyBlock;
	}
}
void Request::_parseChunkedBody(const std::string& bodyBlock) {
	size_t pos = 0;
	while (pos < bodyBlock.length()) {
		size_t endOfLine = bodyBlock.find("\r\n", pos);
		if (endOfLine == std::string::npos) break; 
		
		std::string hexStr = bodyBlock.substr(pos, endOfLine - pos);
		long chunkSize = std::strtol(hexStr.c_str(), NULL, 16);
		if (chunkSize == 0) break;
		
		pos = endOfLine + 2;
		if (pos + chunkSize > bodyBlock.length()) throw std::runtime_error("Chunk incompleto");
		
		_body += bodyBlock.substr(pos, chunkSize);
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
std::map<std::string, std::string> Request::getHeaders() const { return _headers; }