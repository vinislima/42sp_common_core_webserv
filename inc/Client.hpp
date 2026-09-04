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

		Client() : bytesSent(0), isReadyToSend(false) {
			lastActivity = time(NULL);
		}
		~Client() {}
		
		void updateActivity() {
			lastActivity = time(NULL);
		}
};

#endif
