/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 10:15:59 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 10:16:57 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpParser.hpp"

HttpParser::HttpParser(const std::string &rawRequest) : raw(rawRequest), pos(0) {}

HTTPRequest HttpParser::parseRequest() {
	HTTPRequest req;
	parseRequestLine(req);
	//parseHeaders(req);
	//parseBody(req);
	return req;
}

