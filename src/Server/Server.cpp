/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:41:18 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:03:02 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void Server::run() {
    while (true) {
        // 1. Poll all registered FDs
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000); // 1s timeout
        if (poll_count < 0) break;

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            int fd = _poll_fds[i].fd;

            // Handle Incoming Data (Reading)
            if (_poll_fds[i].revents & POLLIN) {
                if (is_listener(fd)) {
                    accept_new_connection(fd);
                } else {
                    handle_client_read(fd, *_clients[fd]);
                }
            }

            // Handle Outgoing Data (Writing)
            if (_poll_fds[i].revents & POLLOUT) {
                handle_client_write(fd, *_clients[fd]);
            }

            // --- State Transitions & Cleanup ---
            Client *c = _clients[fd];
            if (!c) continue;

            // If we just finished reading and need to process
            if (c->state == STATE_PROCESSING) {
                process_request(*c); 
                // Switch this FD's interest from Read to Write
                _poll_fds[i].events = POLLOUT; 
            }

            // If the transaction is done or an error occurred
            if (c->state == STATE_DONE || c->state == STATE_ERROR) {
                std::cout << "Closing connection on FD " << fd << std::endl;
                close(fd);
                delete _clients[fd];
                _clients.erase(fd);
                _poll_fds.erase(_poll_fds.begin() + i);
                --i; // Adjust index because we removed an element
            }
        }
    }
}

void Server::process_request(Client &c) {
    // 1. Create the Request Object (this triggers the parsing)
    Request req(c.request_buffer);

    // 2. Logic Check: What did they ask for?
    std::cout << "Method: " << req.get_method() << " for " << req.get_path() << std::endl;
    std::cout << "User-Agent: " << req.get_header("User-Agent") << std::endl;

    // 3. Hand off to Response Generator (We'll build this class next)
    // Response res(req);
    // c.response_buffer = res.get_raw_response();
    
    // For now, simple manual response
    c.response_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World";
    c.state = STATE_WRITING_RESPONSE;
}

bool Server::is_listener(int fd) {
    
    for (size_t i = 0; i < _listen_fds.size(); ++i) {
        if (_listen_fds[i] == fd)
            return true;
    }
    return false;
}

void Server::setup_server(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("Socket creation failed");

    // 1. Allow immediate reuse of the port
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Set to non-blocking
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Bind failed");

    if (listen(listen_fd, 128) < 0)
        throw std::runtime_error("Listen failed");

    // Add to our poll list
    pollfd pfd = {listen_fd, POLLIN, 0};
    _poll_fds.push_back(pfd);
    _listen_fds.push_back(listen_fd);
    
    std::cout << "Server listening on port " << port << std::endl;
}

void Server::accept_new_connection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) return;

    // VERY IMPORTANT: New client must also be non-blocking
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // Create our state-tracking object
    _clients[client_fd] = new Client(client_fd);

    // Add to poll list
    pollfd pfd = {client_fd, POLLIN, 0};
    _poll_fds.push_back(pfd);
    
    std::cout << "New client connected on FD " << client_fd << std::endl;
}

/*
Non-blocking sockets: By using O_NONBLOCK, if we call recv() and there is no data, the function returns immediately instead of making the whole server wait.

The Map: Using _clients[client_fd] allows us to keep track of which client is at what stage of the HTTP request.
*/

void Server::handle_client_read(int fd, Client &c) {
    char buffer[4096];
    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cout << "Client FD " << fd << " disconnected." << std::endl;
        } else {
            std::cerr << "Recv error on FD " << fd << std::endl;
        }
        c.state = STATE_DONE; // Signal to clean up this client
        return;
    }

    buffer[bytes_read] = '\0';
    c.request_buffer.append(buffer, bytes_read);
    c.last_activity = time(NULL); // Reset timeout timer

    // State Machine Logic
    if (c.state == STATE_READING_REQUEST) {
        // Check if we have finished reading the header
        if (c.request_buffer.find("\r\n\r\n") != std::string::npos) {
            std::cout << "Header fully received for FD " << fd << std::endl;
            
            // In a real Webserv, you'd parse the header here to see 
            // if there is a Content-Length for a Body.
            // For now, let's move straight to processing.
            c.state = STATE_PROCESSING;
        }
    }
}

void Server::handle_client_write(int fd, Client &c) {
    if (c.response_buffer.empty()) return;

    // send() returns the number of bytes actually accepted by the kernel
    ssize_t bytes_sent = send(fd, c.response_buffer.c_str(), c.response_buffer.size(), 0);

    if (bytes_sent > 0) {
        // Remove the bytes that were successfully sent from the buffer
        c.response_buffer.erase(0, bytes_sent);
        c.last_activity = time(NULL);

        // If the buffer is now empty, we are finished with this response
        if (c.response_buffer.empty()) {
            std::cout << "Response fully sent to FD " << fd << std::endl;
            c.state = STATE_DONE; 
        }
    } else if (bytes_sent == -1) {
        std::cerr << "Send error on FD " << fd << std::endl;
        c.state = STATE_ERROR;
    }
}

void Server::process_request(Client &c) {
    // 1. Parse the incoming raw string
    Request req(c.request_buffer);

    // 2. Generate the appropriate response
    Response res(req);

    // 3. Feed the response back to the client buffer
    c.response_buffer = res.get_raw_response();
    
    // 4. Update the state machine
    c.state = STATE_WRITING_RESPONSE;
}

