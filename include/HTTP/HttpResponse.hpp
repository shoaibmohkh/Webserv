/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 09:19:31 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 09:20:19 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP


#include <string>
#include <map>


class HttpResponse {
    public:
        int                                statusCode; // 200, 404, etc.
        std::string                        statusMsg;  // OK, Not Found, etc.
        std::string                        body;       // response body (if any)
        std::map<std::string, std::string> headers;    // headers (name, value)
};

#endif