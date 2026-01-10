/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 10:15:59 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/10 21:04:25 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpParser.hpp"

Logger logger;

HttpParser::HttpParser(std::string &req) : _request(req), _pos(0){}

HTTPRequest HttpParser::parseRequest(){
	parseRequestLine();
//	parseHeaders();

	return httpRequest;
}

void HttpParser::parseRequestLine(){

	//the request line template ->  METHOD SP URI SP VERSION

	std::string line = getLine();
	
	size_t firstSpace = line.find(' '); // the index of the first space 
	size_t secondSpace = line.find(' ', firstSpace + 1); // the index of the second space

	if (firstSpace == std::string::npos || secondSpace == std::string::npos)
		logger.log(ERROR, "parseRequestLine - HttpParser", "No Spaces Found in the Request line");
	
	std::string method = line.substr(0, firstSpace);
	httpRequest.uri = line.substr(firstSpace + 1 , (secondSpace - firstSpace - 1));
	httpRequest.version = line.substr(secondSpace + 1);
	
	if (method.compare("GET") == 0)
		httpRequest.method = HTTP_GET;
	else if (method.compare("DELETE") == 0)
		httpRequest.method = HTTP_DELETE;
	else if (method.compare("POST") == 0)
		httpRequest.method = HTTP_POST;
	else {
		httpRequest.method = HTTP_UNKNOWN;
		// 400 - bad request
		logger.log(ERROR, "parseRequestLine - HttpParser", "UNKNOWN HTTP METHOD!");
	}
		
	std::cout << "The Method is " + method + "\n" + "The uri is " + httpRequest.uri + "\n" + "The version is " + httpRequest.version<<std::endl; 
	return ;
	
}

// void HttpParser::parseHeaders()
// {
	
// }

std::string HttpParser::getLine(){
	
	if (_pos >= _request.size())
		return "";
	
	size_t start = _pos;

	while (_pos < _request.size() && _request[_pos] != '\n')
		_pos++;
		
	size_t end = _pos;

	if (_pos < _request.size())
		_pos++;

	if (end > start && _request[end - 1] == '\r')
		end -= 1;
	else 
		logger.log(ERROR, "getLine - Parser", "Line Doest end with CRLF WHICH VIOLATES THE HTTP RULES");
	
	return _request.substr(start, (end - start));
}
