/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 19:39:45 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:16:19 by eaqrabaw         ###   ########.fr       */
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
        int         fd;
        int         cgi_pipe_fd; // The pipe we read from
        pid_t       cgi_pid;      // The child process ID
        e_state     state;
        std::string response_buffer;
    
};

#endif