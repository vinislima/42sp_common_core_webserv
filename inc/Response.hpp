/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 20:11:00 by vinda-si          #+#    #+#             */
/*   Updated: 2026/08/18 23:07:35 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <sstream>
#include <map>
#include "Request.hpp"
#include "ServerConfig.hpp"

class Response {
private:
	int									_statusCode;
	std::string							_body;
	std::string 						_rawResponse;
	std::map<std::string, std::string>	_headers;
	std::map<int, std::string>			_statusMessages;

	void		_initStatusMessages();
	void		_generateRawResponse();
	std::string _getContentType(const std::string& path) const;
	const		LocationConfig* _getBestMatchLocation(const std::string& uri, const ServerConfig& config) const;

public:
	Response();
	~Response();

	void	setStatusCode(int code);
	void	setHeader(const std::string& key, const std::string& value);
	void	setBody(const std::string& body);

	void	build(const Request& req, const ServerConfig& config);

	std::string getRawResponse() const;
};

#endif
