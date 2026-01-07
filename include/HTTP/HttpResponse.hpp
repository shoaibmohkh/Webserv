/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 09:19:31 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 09:54:46 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP


#include <string>
#include <map>


class HTTPResponse {
	public:
		int status_code;                          // 200, 404, etc.
		std::string reason_phrase;                // "OK", "Not Found", etc.
		std::string body;                         // Response body
		std::map<std::string, std::string> headers; // Headers as key-value map
	};

#endif