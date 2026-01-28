/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http10Serializer.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:19:55 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/01/28 11:19:56 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/HTTP/http10/Http10Serializer.hpp"

#include <sstream>
#include <map>

static std::string to_dec(size_t v)
{
    std::ostringstream os;
    os << (unsigned long)v;
    return os.str();
}

namespace http10
{
    std::string makeError(int code, const char* msg)
    {
        std::string body(msg ? msg : "");
        if (body.empty() || body[body.size() - 1] != '\n')
            body += "\n";

        const char* reason = "Error";
        if (code == 400) reason = "Bad Request";
        else if (code == 404) reason = "Not Found";
        else if (code == 405) reason = "Method Not Allowed";
        else if (code == 411) reason = "Length Required";
        else if (code == 413) reason = "Payload Too Large";
        else if (code == 505) reason = "HTTP Version Not Supported";
        else if (code == 500) reason = "Internal Server Error";

        std::ostringstream oss;
        oss << "HTTP/1.0 " << code << " " << reason << "\r\n";
        oss << "Content-Type: text/plain\r\n";
        oss << "Content-Length: " << to_dec(body.size()) << "\r\n";
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }

    std::string serializeClose(const HTTPResponse& res)
    {
        std::map<std::string, std::string> headers = res.headers;

        headers["Connection"] = "close";
        headers["Content-Length"] = to_dec(res.body.size());
        if (headers.find("Content-Type") == headers.end())
            headers["Content-Type"] = "text/plain";

        int code = res.status_code;
        std::string reason = res.reason_phrase;
        if (reason.empty())
        {
            if (code == 200) reason = "OK";
            else if (code == 201) reason = "Created";
            else if (code == 204) reason = "No Content";
            else if (code == 301) reason = "Moved Permanently";
            else if (code == 302) reason = "Found";
            else if (code == 400) reason = "Bad Request";
            else if (code == 403) reason = "Forbidden";
            else if (code == 404) reason = "Not Found";
            else if (code == 405) reason = "Method Not Allowed";
            else if (code == 411) reason = "Length Required";
            else if (code == 413) reason = "Payload Too Large";
            else if (code == 500) reason = "Internal Server Error";
            else if (code == 505) reason = "HTTP Version Not Supported";
            else reason = "OK";
        }

        std::ostringstream oss;
        oss << "HTTP/1.0 " << code << " " << reason << "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it)
            oss << it->first << ": " << it->second << "\r\n";
        oss << "\r\n";

        std::string out = oss.str();
        if (!res.body.empty())
            out.append(&res.body[0], res.body.size());

        return out;
    }
}
