/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 12:02:15 by eaqrabaw          #+#    #+#             */
/*   Updated: 2025/12/14 12:02:15 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

enum HTTPMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_UNKNOWN
};

class HTTPRequest {
    public:
        HTTPMethod method;                       // GET, POST, DELETE as enum
        std::string uri;                         // "/path/to/resource"
        std::string version;                     // "HTTP/1.0" or "HTTP/1.1"
        std::vector<char> body;                        // Request body (if any)
        std::map<std::string, std::string> headers; // Headers as key-value map
        std::string host;                        // Extracted host from headers
        int port;                                // Extracted port from Host header
        bool keepAlive;                         // true if Connection: keep-alive, false otherwise

    void set_body(const std::string& text)
    {
        body.assign(text.begin(), text.end());
    }
};


#endif