/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 09:41:21 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 09:41:22 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

class Client {
public:
    Request req;
    std::string responseBuffer;
    size_t bytesSent;
    bool isReadyToSend;
    time_t lastActivity;
    
    bool isCgi;
    pid_t cgiPid;
    int cgiReadFd;
    int cgiWriteFd;
    std::string cgiOutput;
    size_t cgiBytesWritten;
    time_t cgiStart;

    // Variáveis de I/O de Arquivo Assíncrono
    bool isFile;
    int fileReadFd;
    int fileWriteFd;
    size_t fileBytesWritten;

    Client() : bytesSent(0), isReadyToSend(false), isCgi(false), cgiPid(-1),
                cgiReadFd(-1), cgiWriteFd(-1), cgiBytesWritten(0), cgiStart(0),
                isFile(false), fileReadFd(-1), fileWriteFd(-1), fileBytesWritten(0) {
        lastActivity = time(NULL);
    }
    
    ~Client() {}

    void updateActivity() {
        lastActivity = time(NULL);
    }
};

#endif