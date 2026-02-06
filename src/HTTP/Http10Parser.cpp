/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http10Parser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:36:45 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/06 18:34:19 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/HTTP/http10/Http10Parser.hpp"

#include <map>
#include <cctype>
#include <sstream>

static bool is_sp(char c) { return (c == ' ' || c == '\t'); }

static std::string trim(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace((unsigned char)s[a])) a++;
    size_t b = s.size();
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

static std::string to_lower(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = (char)std::tolower((unsigned char)s[i]);
    return s;
}

static HTTPMethod method_from_token(const std::string& m)
{
    if (m == "GET")    return HTTP_GET;
    if (m == "POST")   return HTTP_POST;
    if (m == "DELETE") return HTTP_DELETE;
    if (m == "HEAD")   return HTTP_HEAD;
    return HTTP_UNKNOWN;
}

static void parse_host_header_value(const std::string& hostValue, std::string& outHost, int& outPort)
{
    std::string v = trim(hostValue);
    if (v.empty())
        return;

    size_t colon = v.find(':');
    if (colon == std::string::npos)
    {
        outHost = v;
        return;
    }

    outHost = v.substr(0, colon);

    std::string p = v.substr(colon + 1);
    if (p.empty())
        return;

    int port = 0;
    for (size_t i = 0; i < p.size(); ++i)
    {
        if (p[i] < '0' || p[i] > '9')
            return;
        port = port * 10 + (p[i] - '0');
        if (port > 65535)
            return;
    }
    if (port > 0)
        outPort = port;
}

static bool parse_content_length_value(const std::string& v, size_t& outLen)
{
    std::string s = trim(v);
    if (s.empty())
        return false;

    size_t n = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            return false;

        size_t digit = (size_t)(s[i] - '0');
        if (n > ((size_t)-1 / 10))
            return false;
        n = n * 10 + digit;
    }

    outLen = n;
    return true;
}

static bool parse_http10_request_line(const std::string& headersPart, HTTPRequest& outReq, int& outErrCode)
{
    outErrCode = 400;

    size_t lineEnd = headersPart.find('\n');
    std::string line = (lineEnd == std::string::npos) ? headersPart : headersPart.substr(0, lineEnd);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    if (line.empty())
        return false;

    size_t i = 0;

    while (i < line.size() && is_sp(line[i])) i++;
    size_t m0 = i;
    while (i < line.size() && !is_sp(line[i])) i++;
    size_t m1 = i;

    while (i < line.size() && is_sp(line[i])) i++;
    size_t u0 = i;
    while (i < line.size() && !is_sp(line[i])) i++;
    size_t u1 = i;

    while (i < line.size() && is_sp(line[i])) i++;
    size_t v0 = i;
    while (i < line.size() && !is_sp(line[i])) i++;
    size_t v1 = i;

    while (i < line.size() && is_sp(line[i])) i++;
    if (i != line.size())
        return false;

    if (m0 == m1 || u0 == u1 || v0 == v1)
        return false;

    std::string method = line.substr(m0, m1 - m0);
    std::string uri    = line.substr(u0, u1 - u0);
    std::string ver    = line.substr(v0, v1 - v0);

    HTTPMethod hm = method_from_token(method);
    if (hm == HTTP_UNKNOWN)
    {
        outErrCode = 405;
        return false;
    }

    if (ver != "HTTP/1.0" && ver != "HTTP/1.1")
    {
        outErrCode = 505;
        return false;
    }

    if (uri.empty() || uri[0] != '/')
        return false;

    outReq.method  = hm;
    outReq.uri     = uri;
    outReq.version = ver;
    return true;
}

namespace http10
{
    bool parseRequest(const std::string& raw, int listenPort, HTTPRequest& outReq, int& outErrCode)
    {
        outErrCode = 400;

        size_t he = raw.find("\r\n\r\n");
        size_t delimLen = 4;

        if (he == std::string::npos)
        {
            he = raw.find("\n\n");
            delimLen = 2;
        }

        std::string headersPart;
        std::string bodyPart;

        if (he == std::string::npos)
        {
            headersPart = raw;
            bodyPart = "";
        }
        else
        {
            headersPart = raw.substr(0, he);
            bodyPart = raw.substr(he + delimLen);
        }

        outReq.headers.clear();
        outReq.body.clear();
        outReq.host = "";
        outReq.port = listenPort;
        outReq.keepAlive = false;

        if (!parse_http10_request_line(headersPart, outReq, outErrCode))
            return false;

        size_t firstNL = headersPart.find('\n');
        if (firstNL != std::string::npos)
        {
            size_t pos = firstNL + 1;
            while (pos < headersPart.size())
            {
                size_t eol = headersPart.find('\n', pos);
                std::string line;

                if (eol == std::string::npos)
                {
                    line = headersPart.substr(pos);
                    pos = headersPart.size();
                }
                else
                {
                    line = headersPart.substr(pos, eol - pos);
                    pos = eol + 1;
                }

                if (!line.empty() && line[line.size() - 1] == '\r')
                    line.erase(line.size() - 1);

                if (line.empty())
                    break;

                size_t c = line.find(':');
                if (c == std::string::npos)
                {
                    outErrCode = 400;
                    return false;
                }

                std::string key = trim(line.substr(0, c));
                std::string val = trim(line.substr(c + 1));

                if (key.empty())
                {
                    outErrCode = 400;
                    return false;
                }
                std::string keyLower = to_lower(key);
                for (std::map<std::string, std::string>::const_iterator it = outReq.headers.begin();
                     it != outReq.headers.end(); ++it)
                {
                    if (to_lower(it->first) == keyLower)
                    {
                        outErrCode = 400;
                        return false;
                    }
                }

                outReq.headers[key] = val;

                if (keyLower == "host")
                    parse_host_header_value(val, outReq.host, outReq.port);
            }
        }

        bool hasCL = false;
        size_t contentLen = 0;

        for (std::map<std::string, std::string>::const_iterator it = outReq.headers.begin();
             it != outReq.headers.end(); ++it)
        {
            std::string k = to_lower(it->first);
            if (k == "transfer-encoding")
            {
                outErrCode = 505;
                return false;
            }
            else if (k == "content-length")
            {
                if (hasCL)
                {
                    outErrCode = 400;
                    return false;
                }
                if (!parse_content_length_value(it->second, contentLen))
                {
                    outErrCode = 400;
                    return false;
                }
                hasCL = true;
            }
        }
        if (!hasCL && outReq.method == HTTP_POST)
        {
            outErrCode = 411;
            return false;
        }
        if (!hasCL)
        {
            if (!bodyPart.empty())
            {
                outErrCode = 400;
                return false;
            }
            outReq.body.clear();
            return true;
        }
        if (bodyPart.size() != contentLen)
        {
            outErrCode = 400;
            return false;
        }

        outReq.body.assign(bodyPart.begin(), bodyPart.end());
        return true;
    }
}
