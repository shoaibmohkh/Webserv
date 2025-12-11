/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 02:18:14 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 02:18:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPTYPES_HPP
#define HTTPTYPES_HPP

#include <string>
#include <map>

enum HTTPMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_UNKNOWN
};

struct HTTPRequest {
    int         port;    // extracted from headers or default part 1/2 and it's the port number only form the host field
    HTTPMethod  method;
    std::string uri;     // /images/cat.png
    std::string version; // HTTP/1.1 , HTTP/1.0
    std::string body;
    std::string host;    // extracted from headers part 2 the host name only not all the host filed
    std::map<std::string, std::string> headers;
};

struct HTTPResponse {
    int         status_code;
    std::string body;
    std::string reason_phrase;
    std::map<std::string, std::string> headers;
};

#endif