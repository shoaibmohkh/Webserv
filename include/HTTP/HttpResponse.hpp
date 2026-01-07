/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 09:19:31 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 09:28:29 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP


#include <string>
#include <map>


class HTTPResponse {
	public:
		int statusCode;                          // 200, 404, etc.
		std::string reasonPhrase;                // "OK", "Not Found", etc.
		std::string body;                        // Response body
		std::map<std::string, std::string> headers; // Headers as key-value map
	};

#endif