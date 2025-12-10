/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 16:47:10 by marvin            #+#    #+#             */
/*   Updated: 2025/12/10 16:47:10 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <map>

class HttpRequest {
    public:
        std::string                        method;  // GET, POST, etc.
        std::string                        uri;     // "path?query" (as received)
        std::string                        version; // HTTP/1.0, HTTP/1.1, etc.
        std::string                        body;    // request body (if any)
        std::map<std::string, std::string> headers; // headers (name, value)
};

class HttpResponse {
    public:
        int                                statusCode; // 200, 404, etc.
        std::string                        statusMsg;  // OK, Not Found, etc.
        std::string                        body;       // response body (if any)
        std::map<std::string, std::string> headers;    // headers (name, value)
};


#endif