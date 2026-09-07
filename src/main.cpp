/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:07:23 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:07:24 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/ConfigParser.hpp"
#include "../inc/Server.hpp" 
#include <iostream>
#include <signal.h>

int main(int argc, char **argv) {
	// Subject: "./webserv [configuration file]" — 0 or 1 arg is valid (no
	// arg falls back to default.conf); anything else is a usage error, not
	// something to silently ignore.
	if (argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [configuration file]" << std::endl;
		return 1;
	}

	signal(SIGPIPE, SIG_IGN);

	std::string configFile = (argc == 2) ? argv[1] : "default.conf";

	try {
		ConfigParser parser(configFile);
		parser.parse();
		std::vector<ServerConfig> configs = parser.getServers();
		
		Server webServer(configs);
		webServer.start();
		
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
	return 0;
}