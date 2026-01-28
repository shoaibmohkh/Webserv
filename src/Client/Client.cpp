/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 19:43:19 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 20:30:35 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

 
#include "Client.hpp"
  
Client::Client(int socket_fd) : 
        fd(socket_fd), 
        state(STATE_READING_REQUEST), 
        last_activity(time(NULL)) {}
        