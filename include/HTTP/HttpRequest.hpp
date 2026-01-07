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

class HttpRequest {
    public:
        std::string                        method;  // GET, POST, etc.
        std::string                        uri;     // "path?query" (as received)
        std::string                        version; // HTTP/1.0, HTTP/1.1, etc.
        std::string                        body;    // request body (if any)
        std::map<std::string, std::string> headers; // headers (name, value)
};




#endif