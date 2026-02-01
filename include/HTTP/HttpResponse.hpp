/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 09:19:31 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/02/01 13:27:22 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP


#include <string>
#include <map>
#include <vector>

class HTTPResponse {
	public:
		int status_code;                          // 200, 404, etc.
		std::string reason_phrase;                // "OK", "Not Found", etc.
		std::vector<char> body;                         // Response body
		std::map<std::string, std::string> headers; // Headers as key-value map
	    bool is_file;
	    std::string file_path;
	
	void set_body(const std::string& text)
    {
        body.assign(text.begin(), text.end());
    }
	void set_body(const std::vector<char>& data)
    {
        body = data;
    }
};

#endif