/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 22:39:29 by vinda-si          #+#    #+#             */
/*   Updated: 2026/09/05 20:52:28 by yvieira-         ###   ########.fr       */
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

		bool isFile;
		int fileReadFd;
		int fileWriteFd;
		std::string fileBuffer;

		

		Client() : bytesSent(0), isReadyToSend(false), isCgi(false), cgiPid(-1), 
               cgiReadFd(-1), cgiWriteFd(-1), cgiBytesWritten(0), isFile(false), fileReadFd(-1), fileWriteFd(-1) {
        lastActivity = time(NULL);
    	}
		~Client() {}
		
		void updateActivity() {
			lastActivity = time(NULL);
		}
};

#endif
