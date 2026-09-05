/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 22:39:29 by vinda-si          #+#    #+#             */
/*   Updated: 2026/08/22 20:40:29 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>


class Client {
	public:
		Request		req;
		std::string	responseBuffer;
		size_t		bytesSent;
		bool		isReadyToSend;
		time_t		lastActivity;
		
		bool isCgi;
    	pid_t cgiPid;
    	int cgiReadFd;
    	int cgiWriteFd;
    	std::string cgiOutput;
    	size_t cgiBytesWritten;

		Client() : bytesSent(0), isReadyToSend(false), isCgi(false), cgiPid(-1), 
               cgiReadFd(-1), cgiWriteFd(-1), cgiBytesWritten(0) {
        lastActivity = time(NULL);
    	}
		~Client() {}
		
		void updateActivity() {
			lastActivity = time(NULL);
		}
};

#endif
