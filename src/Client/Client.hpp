/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 19:39:45 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 20:51:32 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>


enum e_state {
    STATE_READING_REQUEST,
    STATE_PROCESSING,
    STATE_WRITING_RESPONSE,
    STATE_DONE,
    STATE_ERROR
};

class Client {
public:
    Client(int socket_fd);
    int             fd;
    e_state         state;
    std::string     request_buffer;  // Raw data read from socket
    std::string     response_buffer; // Data waiting to be sent
    time_t          last_activity;   // For timeout management
};

#endif