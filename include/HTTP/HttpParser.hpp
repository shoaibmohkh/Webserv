/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 10:14:01 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 10:15:17 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "HttpRequest.hpp"
#include <string>

class HttpParser {
	public:
		HttpParser(const std::string &rawRequest);
		HTTPRequest parseRequest();
	
	private:
		std::string raw;
		size_t pos;
		
		// Parsing functions
		HTTPMethod parseMethod(const std::string &methodStr);
		void parseRequestLine(HTTPRequest &req);
		void parseHeaders(HTTPRequest &req);
		void parseBody(HTTPRequest &req);
	
		// Helpers functions
		std::string getLine();
	};
	
#endif