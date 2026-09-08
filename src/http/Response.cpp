/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:47 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 20:55:52 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/Response.hpp"
#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cctype>
#include <vector>
#include <algorithm>

Response::Response() : _statusCode(200), _cgiPid(-1), _cgiReadFd(-1), _cgiWriteFd(-1), _fileReadFd(-1), _fileWriteFd(-1) {
	_initStatusMessages();
}

Response::~Response() {}

void Response::_initStatusMessages() {
	_statusMessages[200] = "OK";
	_statusMessages[201] = "Created";
	_statusMessages[204] = "No Content";
	_statusMessages[301] = "Moved Permanently";
	_statusMessages[302] = "Found";
	_statusMessages[303] = "See Other";
	_statusMessages[307] = "Temporary Redirect";
	_statusMessages[308] = "Permanent Redirect";
	_statusMessages[400] = "Bad Request";
	_statusMessages[403] = "Forbidden";
	_statusMessages[404] = "Not Found";
	_statusMessages[405] = "Method Not Allowed";
	_statusMessages[413] = "Payload Too Large";
	_statusMessages[431] = "Request Header Fields Too Large";
	_statusMessages[500] = "Internal Server Error";
	_statusMessages[501] = "Not Implemented";
}

std::string Response::_getContentType(const std::string& path) const {
	// A previous version matched by path.find(".html") — a substring search
	// anywhere in the path, not the actual extension (so "foo.htmlbak" would
	// also match ".html", and ".HTML" wouldn't match at all). This looks up
	// the real extension (after the last '/', lowercased) in a proper table.
	static std::map<std::string, std::string> mimeTypes;
	if (mimeTypes.empty()) {
		mimeTypes[".html"] = "text/html";
		mimeTypes[".htm"] = "text/html";
		mimeTypes[".css"] = "text/css";
		mimeTypes[".js"] = "application/javascript";
		mimeTypes[".json"] = "application/json";
		mimeTypes[".xml"] = "application/xml";
		mimeTypes[".txt"] = "text/plain";
		mimeTypes[".pdf"] = "application/pdf";
		mimeTypes[".zip"] = "application/zip";
		mimeTypes[".png"] = "image/png";
		mimeTypes[".jpg"] = "image/jpeg";
		mimeTypes[".jpeg"] = "image/jpeg";
		mimeTypes[".gif"] = "image/gif";
		mimeTypes[".svg"] = "image/svg+xml";
		mimeTypes[".ico"] = "image/x-icon";
		mimeTypes[".webp"] = "image/webp";
		mimeTypes[".mp4"] = "video/mp4";
		mimeTypes[".webm"] = "video/webm";
		mimeTypes[".woff"] = "font/woff";
		mimeTypes[".woff2"] = "font/woff2";
		mimeTypes[".ttf"] = "font/ttf";
	}

	size_t lastSlash = path.find_last_of('/');
	size_t dotPos = path.find_last_of('.');
	bool hasExtension = dotPos != std::string::npos && (lastSlash == std::string::npos || dotPos > lastSlash);

	std::string ext;
	if (hasExtension) {
		ext = path.substr(dotPos);
		for (size_t i = 0; i < ext.length(); ++i) {
			ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
		}
	}

	std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
	return (it != mimeTypes.end()) ? it->second : "text/plain";
}

void Response::setStatusCode(int code) {
	_statusCode = code;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
}

void Response::_generateRawResponse() {
	std::ostringstream ss;
	ss << "HTTP/1.1 " << _statusCode << " ";
	if (_statusMessages.find(_statusCode) != _statusMessages.end())
		ss << _statusMessages[_statusCode];
	else
		ss << "Unknown Status";
	ss << "\r\n";
		
	std::ostringstream lengthStream;
	lengthStream << _body.length();
	_headers["Content-Length"] = lengthStream.str();
		
	if (_headers.find("Content-Type") == _headers.end() && !_body.empty()) {
		_headers["Content-Type"] = "text/plain";
	}
		
	for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
		ss << it->first << ": " << it->second << "\r\n";
	}
	ss << "\r\n";
	ss << _body;
	_rawResponse = ss.str();
}

// Collapses "." and ".." from an HTTP URI (always absolute, starts with
// "/"), rejecting any ".." that tries to go above the root — e.g.
// "/../../../../etc/passwd" or "/files/../../etc/passwd" become invalid
// instead of escaping the directory configured in `root`. Returns false
// when the URI is a Path Traversal attempt.
bool Response::_normalizeUri(const std::string& uri, std::string& out) const {
	std::vector<std::string> segments;
	bool trailingSlash = !uri.empty() && uri[uri.length() - 1] == '/';
	size_t pos = 0;

	while (pos <= uri.length()) {
		size_t next = uri.find('/', pos);
		std::string segment = (next == std::string::npos) ? uri.substr(pos) : uri.substr(pos, next - pos);

		if (segment == "..") {
			if (segments.empty()) return false; // tried to go above the root
			segments.pop_back();
		} else if (!segment.empty() && segment != ".") {
			segments.push_back(segment);
		}

		if (next == std::string::npos) break;
		pos = next + 1;
	}

	out = "/";
	for (size_t i = 0; i < segments.size(); ++i) {
		out += segments[i];
		if (i + 1 < segments.size()) out += "/";
	}
	// Preserve the trailing slash (e.g. "/files/") — location and
	// uploadStore depend on it to match, and always stripping it would break that.
	if (trailingSlash && out != "/") out += "/";
	return true;
}

// Manually parses a multipart/form-data body (RFC 7578): finds the first
// part that has "filename=" in its Content-Disposition (i.e. it's a file,
// not a regular form field), splits that part's headers from its binary
// content and returns both. Without this, the boundary and each part's
// headers would end up in the file saved to disk.
bool Response::_parseMultipart(const std::string& body, const std::string& boundary,
								std::string& outFilename, std::string& outContent) const {
	std::string delimiter = "--" + boundary;
	size_t pos = body.find(delimiter);
	if (pos == std::string::npos) return false;

	while (pos != std::string::npos) {
		pos += delimiter.length();

		// "--" right after the boundary marks the end of the multipart (final delimiter).
		if (body.compare(pos, 2, "--") == 0) break;

		if (body.compare(pos, 2, "\r\n") == 0) pos += 2;

		size_t nextDelim = body.find(delimiter, pos);
		std::string part = (nextDelim == std::string::npos) ? body.substr(pos) : body.substr(pos, nextDelim - pos);

		size_t headerEnd = part.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			std::string partHeaders = part.substr(0, headerEnd);
			std::string content = part.substr(headerEnd + 4);

			// remove the trailing "\r\n" that precedes the next boundary
			if (content.length() >= 2 && content.compare(content.length() - 2, 2, "\r\n") == 0) {
				content = content.substr(0, content.length() - 2);
			}

			size_t cdPos = partHeaders.find("Content-Disposition:");
			if (cdPos != std::string::npos) {
				size_t namePos = partHeaders.find("filename=\"", cdPos);
				if (namePos != std::string::npos) {
					namePos += 10;
					size_t endPos = partHeaders.find("\"", namePos);
					if (endPos != std::string::npos) {
						outFilename = partHeaders.substr(namePos, endPos - namePos);
						outContent = content;
						return true;
					}
				}
			}
		}

		pos = nextDelim;
	}
	return false;
}

std::string Response::_getFallbackHTML(int code) const {
	std::ostringstream oss;
	std::string msg = "Unknown Error";
	std::map<int, std::string>::const_iterator it = _statusMessages.find(code);
	if (it != _statusMessages.end()) {
		msg = it->second;
	}
	oss << "<!DOCTYPE html>\n<html>\n<head><title>" << code << " " << msg << "</title></head>\n"
		<< "<body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; text-align: center; padding: 50px;\">\n"
		<< "  <h1 style=\"color: #ff5555;\">" << code << " - " << msg << "</h1>\n"
		<< "  <hr>\n"
		<< "  <p>Webserv / C++98 Fallback</p>\n"
		<< "</body>\n</html>";
	return oss.str();
}

void Response::_buildErrorPage(int code, const ServerConfig& config) {
	setStatusCode(code);
	bool customPageLoaded = false;
		
	std::map<int, std::string> errorPages = config.getErrorPages();
	std::map<int, std::string>::const_iterator it = errorPages.find(code);
		
	if (it != errorPages.end()) {
		std::string errorUri = it->second;
		std::string root = config.getRoot();
		if (root.empty()) {
			root = "./www";
		}
		std::string filepath = root + errorUri;
		
		// Asynchronous I/O for the error page!
		int fd = open(filepath.c_str(), O_RDONLY);
		if (fd >= 0) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
			fcntl(fd, F_SETFD, FD_CLOEXEC); // don't leak this fd into a future CGI child's fork()+execve()
			this->_fileReadFd = fd;

			struct stat path_stat;
			stat(filepath.c_str(), &path_stat);
			
			std::ostringstream ss;
			ss << "HTTP/1.1 " << code << " " << _statusMessages[code] << "\r\n";
			ss << "Content-Length: " << path_stat.st_size << "\r\n";
			ss << "Content-Type: " << _getContentType(filepath) << "\r\n\r\n";
			_rawResponse = ss.str();
			customPageLoaded = true;
		} else {
			std::cout << "[WARN] Nao foi possivel carregar error_page customizada em: " << filepath << ". Usando fallback.\n";
		}
	}
		
	if (!customPageLoaded) {
		setBody(_getFallbackHTML(code));
		setHeader("Content-Type", "text/html");
		_generateRawResponse();
	}
}

void Response::build(Request& req, const ServerConfig& config) {
	_headers.clear();
	_body.clear();
	_rawResponse.clear();

	if (req.getErrorCode() != 0) {
		_buildErrorPage(req.getErrorCode(), config);
		return;
	}

	setStatusCode(200);

	// 501 vs 405: a method this server never implements at all (PATCH, PUT,
	// TRACE, ...) is 501 Not Implemented regardless of location/allow_methods
	// — even a location with no allow_methods restriction (which lets any
	// method through the isMethodAllowed() check below) can't actually serve
	// it, since there is no code path for it past this point. 405 is reserved
	// for a method this server DOES implement but that specific route forbids.
	if (req.getMethod() != "GET" && req.getMethod() != "POST" && req.getMethod() != "DELETE") {
		_buildErrorPage(501, config);
		return;
	}

	std::string cleanUri = req.getUri();
	size_t queryPos = cleanUri.find('?');
	if (queryPos != std::string::npos) {
		cleanUri = cleanUri.substr(0, queryPos);
	}

	// Path Traversal: normalize BEFORE location matching, so location and
	// the filesystem always see the same URI, already stripped of "..".
	std::string normalizedUri;
	if (!_normalizeUri(cleanUri, normalizedUri)) {
		_buildErrorPage(403, config);
		return;
	}
	cleanUri = normalizedUri;

	const LocationConfig* loc = config.getBestMatchLocation(cleanUri);

	// Without a matching location, the implicit policy is GET-only: a
	// config can (and default.conf/second.conf/third.conf now do) add a
	// catch-all `location / { allow_methods GET; }` to cover this, but the
	// server itself shouldn't rely on every .conf remembering that — a
	// config missing a root location previously let POST/DELETE through
	// with no check at all (e.g. DELETE / could remove the site root).
	bool methodAllowed = loc ? loc->isMethodAllowed(req.getMethod()) : (req.getMethod() == "GET");
	if (!methodAllowed) {
		_buildErrorPage(405, config);
		return;
	}

	if (loc && loc->getRedirectCode() != 0) {
		int rCode = loc->getRedirectCode();
		setStatusCode(rCode);
		setHeader("Location", loc->getRedirectUrl());
		std::ostringstream oss;
		oss << "<html>\n<head><title>" << rCode << " " << _statusMessages[rCode] << "</title></head>\n"
			<< "<body style=\"background-color: #282a36; color: #f8f8f2; text-align: center; padding: 50px;\">\n"
			<< "  <h1>" << rCode << " - " << _statusMessages[rCode] << "</h1>\n"
			<< "  <p>Redirecionando para <a style=\"color: #8be9fd;\" href=\"" << loc->getRedirectUrl() << "\">" 
			<< loc->getRedirectUrl() << "</a></p>\n"
			<< "</body>\n</html>";
		setBody(oss.str());
		setHeader("Content-Type", "text/html");
		_generateRawResponse();
		return;
	}

	std::string root = (loc && !loc->getRoot().empty()) ? loc->getRoot() : config.getRoot();

	// autoindex is a bool (no possible "empty" state), so it only inherits
	// from the server when the location doesn't define the directive at all
	// (AUTOINDEX_INHERIT) — unlike root/index above, which use "empty" as
	// the inheritance sentinel.
	bool autoindex;
	if (loc && loc->getAutoindexState() != LocationConfig::AUTOINDEX_INHERIT) {
		autoindex = (loc->getAutoindexState() == LocationConfig::AUTOINDEX_ON);
	} else {
		autoindex = config.getAutoindex();
	}

	std::vector<std::string> indexFiles = (loc && !loc->getIndex().empty()) ? loc->getIndex() : config.getIndex();

	if (root.empty()) {
		root = "./www";
	}

	std::string filepath = root + cleanUri;

	if (filepath != root && filepath[filepath.length() - 1] == '/') {
		filepath = filepath.substr(0, filepath.length() - 1);
	}

	std::string cgiExt = (loc) ? loc->getCgiExt() : "";
	std::string cgiPath = (loc) ? loc->getCgiPath() : "";

	if (!cgiExt.empty() && filepath.length() >= cgiExt.length()) {
		if (filepath.substr(filepath.length() - cgiExt.length()) == cgiExt) {
			std::cout << "[CGI] Arquivo " << cgiExt << " detectado! Redirecionando para execucao.\n";
			_handleCGI(filepath, cgiPath, req, config);
			return;
		}
	}

	std::cout << "[DEBUG] Tentando ler do disco o caminho: " << filepath << "\n";

	if (req.getMethod() == "GET") {
		struct stat path_stat;
		if (stat(filepath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
			bool indexFound = false;
			for (size_t i = 0; i < indexFiles.size(); ++i) {
				std::string indexPath = filepath + "/" + indexFiles[i];
				if (access(indexPath.c_str(), F_OK) == 0) {
					filepath = indexPath;
					indexFound = true;
					break;
				}
			}
			if (!indexFound) {
				if (autoindex) {
					DIR* dir = opendir(filepath.c_str());
					if (dir != NULL) {
						// readdir() returns entries in filesystem/inode order (not
						// alphabetical) and includes "." and "..". Only "." was being
						// filtered — ".." was left in, so the listing offered a link
						// to browse up out of the directory being listed. Collect
						// names first, filter both, and sort before rendering.
						std::vector<std::string> entries;
						struct dirent* entry;
						while ((entry = readdir(dir)) != NULL) {
							std::string name = entry->d_name;
							if (name == "." || name == "..") continue;
							entries.push_back(name);
						}
						closedir(dir);
						std::sort(entries.begin(), entries.end());

						std::string autoindexHtml = "<!DOCTYPE html><html><head><title>Index of " + req.getUri() + "</title></head>";
						autoindexHtml += "<body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; padding: 20px;\">";
						autoindexHtml += "<h1 style=\"color: #50fa7b;\">Index of " + req.getUri() + "</h1><hr><ul>";

						for (size_t idx = 0; idx < entries.size(); ++idx) {
							std::string name = entries[idx];

							std::string link = req.getUri();
							if (link[link.length() - 1] != '/') link += "/";
							link += name;

							std::string fullPath = filepath + "/" + name;
							struct stat item_stat;
							if (stat(fullPath.c_str(), &item_stat) == 0 && S_ISDIR(item_stat.st_mode)) {
								name += "/";
							}

							autoindexHtml += "<li style=\"margin: 5px 0;\"><a style=\"color: #8be9fd; text-decoration: none; font-size: 1.2rem;\" href=\"" + link + "\">" + name + "</a></li>";
						}
						autoindexHtml += "</ul><hr><p style=\"color: #6272a4;\">Webserv / C++98</p></body></html>";
						
						setStatusCode(200);
						setBody(autoindexHtml);
						setHeader("Content-Type", "text/html");
						_generateRawResponse();
						return;
					} else {
						_buildErrorPage(403, config);
						return;
					}
				} else {
					_buildErrorPage(403, config);
					return;
				}
			}
		}

		int fd = open(filepath.c_str(), O_RDONLY);
		if (fd >= 0) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
			fcntl(fd, F_SETFD, FD_CLOEXEC); // don't leak this fd into a future CGI child's fork()+execve()
			this->_fileReadFd = fd;

			struct stat file_stat;
			stat(filepath.c_str(), &file_stat);
			
			std::ostringstream ss;
			ss << "HTTP/1.1 200 OK\r\n";
			ss << "Content-Length: " << file_stat.st_size << "\r\n";
			ss << "Content-Type: " << _getContentType(filepath) << "\r\n\r\n";
			_rawResponse = ss.str();
		} else {
			_buildErrorPage(404, config);
			return;
		}

	} else if (req.getMethod() == "POST") {
		std::string uploadStore = (loc) ? loc->getUploadStore() : "";
		if (uploadStore.empty()) {
			std::cout << "[DEBUG] Post recusado: Nenhum upload_store configurado para a rota " << req.getUri() << ".\n";
			_buildErrorPage(403, config);
			return;
		}
		if (uploadStore[uploadStore.length() - 1] == '/') {
			uploadStore = uploadStore.substr(0, uploadStore.length() - 1);
		}

		struct stat uploadStat;
		if (stat(uploadStore.c_str(), &uploadStat) != 0 || !S_ISDIR(uploadStat.st_mode)) {
			std::cout << "[ERRO] POST falhou: Diretorio de upload (" << uploadStore << ") nao existe no disco.\n";
			_buildErrorPage(500, config);
			return;
		}

		std::string filename = "";
		std::string uploadContent = req.getBody();

		std::string contentType = req.getHeader("Content-Type");
		size_t boundaryPos = contentType.find("boundary=");

		if (contentType.find("multipart/form-data") != std::string::npos && boundaryPos != std::string::npos) {
			// The Content-Disposition of a real upload (via a browser <form>,
			// or curl -F) lives INSIDE the body, per multipart part — never
			// as a top-level HTTP header. We manually parse the boundary to
			// extract just the binary content of the part that is the file.
			std::string boundary = contentType.substr(boundaryPos + 9);
			if (!boundary.empty() && boundary[0] == '"') {
				boundary = boundary.substr(1);
				size_t closingQuote = boundary.find('"');
				if (closingQuote != std::string::npos) boundary = boundary.substr(0, closingQuote);
			} else {
				size_t semiPos = boundary.find(';');
				if (semiPos != std::string::npos) boundary = boundary.substr(0, semiPos);
			}

			std::string extractedContent;
			if (_parseMultipart(req.getBody(), boundary, filename, extractedContent)) {
				uploadContent = extractedContent;
			} else {
				std::cout << "[ERRO] multipart/form-data mal formado ou sem parte de arquivo.\n";
				_buildErrorPage(400, config);
				return;
			}
		} else {
			// Raw upload (no multipart), e.g. curl -X POST --data-binary @file
			// with a Content-Disposition sent manually as a top-level header.
			std::string contentDisposition = req.getHeader("Content-Disposition");
			if (!contentDisposition.empty()) {
				size_t namePos = contentDisposition.find("filename=\"");
				if (namePos != std::string::npos) {
					namePos += 10;
					size_t endPos = contentDisposition.find("\"", namePos);
					if (endPos != std::string::npos) {
						filename = contentDisposition.substr(namePos, endPos - namePos);
					}
				}
			}
		}

		// No filename from Content-Disposition/multipart: for a raw POST
		// (e.g. curl -X POST --data "..." .../files/name.txt — the exact
		// command the subject itself suggests, and what TESTES.md's upload
		// walkthrough demonstrates), fall back to the request URI as the
		// target filename. Without this, every raw upload landed under an
		// autogenerated upload_<timestamp>.bin regardless of the URL used,
		// so a GET back on the URL the client just POSTed to would 404.
		if (filename.empty()) {
			size_t uriSlash = cleanUri.find_last_of('/');
			std::string uriTail = (uriSlash == std::string::npos) ? cleanUri : cleanUri.substr(uriSlash + 1);
			if (!uriTail.empty()) filename = uriTail;
		}

		// filename comes from the client — it can never contain a directory
		// separator, otherwise the upload also becomes a Path Traversal (e.g.
		// filename="../../etc/cron.d/x"). Keep only the last path component.
		size_t lastSlash = filename.find_last_of('/');
		if (lastSlash != std::string::npos) {
			filename = filename.substr(lastSlash + 1);
		}

		if (filename.empty() || filename == "." || filename == "..") {
			std::ostringstream oss;
			oss << "upload_" << time(NULL) << ".bin";
			filename = oss.str();
		}

		// Server.cpp writes to disk from req.getBody() — it needs to be
		// updated with the content already stripped of the multipart envelope.
		req.setBody(uploadContent);

		std::string fullUploadPath = uploadStore + "/" + filename;

		int fd = open(fullUploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd >= 0) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
			fcntl(fd, F_SETFD, FD_CLOEXEC); // don't leak this fd into a future CGI child's fork()+execve()
			this->_fileWriteFd = fd;

			std::string locationHeader = req.getUri();
			if (locationHeader[locationHeader.length() - 1] != '/') {
				locationHeader += "/";
			}
			locationHeader += filename;
			
			std::ostringstream responseHtml;
			responseHtml << "<html><body style=\"font-family: monospace; background-color: #282a36; color: #f8f8f2; padding: 20px;\">\n"
						 << "<h1 style='color: #50fa7b;'>201 - Arquivo Salvo (Created) via poll()</h1>\n"
						 << "<p>Caminho no disco: " << fullUploadPath << "</p>\n"
						 << "</body></html>";
						 
			std::ostringstream ss;
			ss << "HTTP/1.1 201 Created\r\n";
			ss << "Location: " << locationHeader << "\r\n";
			ss << "Content-Length: " << responseHtml.str().length() << "\r\n";
			ss << "Content-Type: text/html\r\n\r\n";
			ss << responseHtml.str();
			_rawResponse = ss.str(); // Response headers + body ready, Server.cpp sends it once the disk write finishes.
		} else {
			std::cout << "[ERRO] Permissao negada ou disco cheio ao gravar em " << fullUploadPath << "\n";
			_buildErrorPage(500, config);
			return;
		}

	} else if (req.getMethod() == "DELETE") {
		if (access(filepath.c_str(), F_OK) != 0) {
			_buildErrorPage(404, config);
			return;
		} else if (std::remove(filepath.c_str()) == 0) {
			setStatusCode(200);
			setBody("<html><body><h1 style='color:green'>200 - OK (Deletado)</h1></body></html>");
			setHeader("Content-Type", "text/html");
		} else {
			_buildErrorPage(403, config);
			return;
		}
		_generateRawResponse();
	} else {
		// Unreachable: the check at the top of build() already sends any
		// method other than GET/POST/DELETE to 501. Kept as a defensive
		// fallback with the semantically correct code, not 405.
		_buildErrorPage(501, config);
		return;
	}
}

std::string Response::getRawResponse() const {
	return _rawResponse;
}

void Response::_handleCGI(const std::string& filepath, const std::string& cgiPath, const Request& req, const ServerConfig& config) {
	std::string uri = req.getUri();
	std::string queryString = "";
	size_t qmPos = uri.find('?');
	if (qmPos != std::string::npos) {
		queryString = uri.substr(qmPos + 1);
		uri = uri.substr(0, qmPos);
	}

	std::string cleanFilepath = filepath;
	size_t filepathQM = cleanFilepath.find('?');
	if (filepathQM != std::string::npos) {
		cleanFilepath = cleanFilepath.substr(0, filepathQM);
	}

	std::string scriptDir = ".";
	std::string scriptName = cleanFilepath;
	size_t lastSlash = cleanFilepath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		scriptDir = cleanFilepath.substr(0, lastSlash);
		scriptName = cleanFilepath.substr(lastSlash + 1);
	}

	std::map<std::string, std::string> envMap;
	envMap["REQUEST_METHOD"] = req.getMethod();
	envMap["QUERY_STRING"] = queryString;
	envMap["SCRIPT_FILENAME"] = scriptName;
	envMap["SCRIPT_NAME"] = uri;
	envMap["SERVER_PROTOCOL"] = req.getVersion();
	envMap["SERVER_SOFTWARE"] = "Webserv/1.0";
	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["REDIRECT_STATUS"] = "200";

	std::ostringstream portStream;
	portStream << config.getPort();
	envMap["SERVER_PORT"] = portStream.str();

	if (!req.getHeader("Content-Length").empty()) envMap["CONTENT_LENGTH"] = req.getHeader("Content-Length");
	else {
		std::ostringstream clStream; clStream << req.getBody().length();
		envMap["CONTENT_LENGTH"] = clStream.str();
	}
	if (!req.getHeader("Content-Type").empty()) envMap["CONTENT_TYPE"] = req.getHeader("Content-Type");

	std::map<std::string, std::string> clientHeaders = req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = clientHeaders.begin(); it != clientHeaders.end(); ++it) {
		std::string key = "HTTP_";
		for (size_t k = 0; k < it->first.length(); ++k) {
			if (it->first[k] == '-') {
				key += '_';
			} else {
				key += std::toupper(static_cast<unsigned char>(it->first[k]));
			}
		}
		envMap[key] = it->second;
	}

	char** envp = new char*[envMap.size() + 1];
	size_t i = 0;
	for (std::map<std::string, std::string>::iterator it = envMap.begin(); it != envMap.end(); ++it) {
		std::string envStr = it->first + "=" + it->second;
		envp[i] = new char[envStr.length() + 1];
		std::strcpy(envp[i], envStr.c_str());
		i++;
	}
	envp[i] = NULL;

	int pipeIn[2], pipeOut[2];
	if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
		for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
		delete[] envp;
		_buildErrorPage(500, config);
		return;
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
		delete[] envp;
		_buildErrorPage(500, config);
		return;
	}

	if (pid == 0) {
		if (chdir(scriptDir.c_str()) < 0) {
			std::cerr << "[CGI FATAL] Falha no chdir para a pasta: " << scriptDir << "\n";
			exit(1);
		}
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);

		char* argv[3];
		argv[0] = const_cast<char*>(cgiPath.c_str());
		argv[1] = const_cast<char*>(scriptName.c_str());
		argv[2] = NULL;

		execve(cgiPath.c_str(), argv, envp);
		std::cerr << "[CGI FATAL] execve falhou ao tentar abrir o interpretador em: " << cgiPath << "\n";
		exit(1);
	} else {
		close(pipeIn[0]);
		close(pipeOut[1]);
		fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
		fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
		// Don't leak these into a DIFFERENT CGI's fork()+execve() while this
		// one is still running (concurrent clients each with their own CGI).
		// dup2() in the child always clears FD_CLOEXEC on the resulting fd
		// regardless of the source, so this has no effect on this CGI's own
		// STDIN/STDOUT — only on fds inherited by an unrelated future child.
		fcntl(pipeIn[1], F_SETFD, FD_CLOEXEC);
		fcntl(pipeOut[0], F_SETFD, FD_CLOEXEC);

		this->_cgiPid = pid;
		this->_cgiWriteFd = pipeIn[1];
		this->_cgiReadFd = pipeOut[0];
		
		for (size_t j = 0; envp[j] != NULL; ++j) delete[] envp[j];
		delete[] envp;
	}
}