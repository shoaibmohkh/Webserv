/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 10:14:01 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/10 21:03:38 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "HttpRequest.hpp"
#include "Logger.hpp"
#include <string>

class HttpParser {
	
	public :
		HttpParser(std::string &req);
	 	HTTPRequest parseRequest();
		
	private :
		const std::string &_request;
		HTTPRequest httpRequest;
		size_t _pos;
		void parseRequestLine();
	//	void parseHeaders();

		// Helper
		std::string getLine();
	};
	
#endif