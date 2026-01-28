/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:41:20 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:03:12 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include "Request.hpp"
#include "Response.hpp"
#include <map>
#include <poll.h>
#include <netinet/in.h>
#include "Client.hpp"

class Server {
private:
    std::vector<int>        _listen_fds;
    std::vector<pollfd>     _poll_fds;
    std::map<int, Client*>  _clients;

public:
    Server();
    ~Server();
    void    setup_server(int port);
    void    run();
    void    accept_new_connection(int listen_fd);
    void    handle_client_read(int fd, Client &c);
    void    handle_client_write(int fd, Client &c);
    bool    is_listener(int fd);
    void    process_request(Client &c);
};

#endif