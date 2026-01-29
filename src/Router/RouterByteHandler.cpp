/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RouterByteHandler.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:17:45 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/01/28 21:45:21 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/RouterByteHandler.hpp"

#include "../../include/config_headers/Tokenizer.hpp"
#include "../../include/config_headers/Parser.hpp"
#include "../../include/Router_headers/Router.hpp"

#include "../../include/HTTP/http10/Http10Parser.hpp"
#include "../../include/HTTP/http10/Http10Serializer.hpp"
#include "../../include/sockets/ListenPort.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <vector>

static bool url_decode_path(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());

    for (size_t i = 0; i < in.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c == '%')
        {
            if (i + 2 >= in.size())
                return false;

            unsigned char h1 = static_cast<unsigned char>(in[i + 1]);
            unsigned char h2 = static_cast<unsigned char>(in[i + 2]);
            if (!std::isxdigit(h1) || !std::isxdigit(h2))
                return false;

            int v1 = (h1 <= '9') ? (h1 - '0') : (std::tolower(h1) - 'a' + 10);
            int v2 = (h2 <= '9') ? (h2 - '0') : (std::tolower(h2) - 'a' + 10);

            out.push_back(static_cast<char>((v1 << 4) | v2));
            i += 2;
        }
        else
            out.push_back(static_cast<char>(c));
    }
    return true;
}

static bool normalize_uri_path(const std::string& rawUri, std::string& out)
{
    std::string path = rawUri;
    size_t q = path.find('?');
    if (q != std::string::npos)
        path = path.substr(0, q);

    if (path.empty() || path[0] != '/')
        path = "/" + path;

    std::vector<std::string> parts;
    std::string cur;

    for (size_t i = 0; i <= path.size(); ++i)
    {
        char ch = (i == path.size()) ? '/' : path[i];
        if (ch == '/')
        {
            if (cur.empty() || cur == ".")
            {
                // skip
            }
            else if (cur == "..")
            {
                if (parts.empty())
                    return false;
                parts.pop_back();
            }
            else
                parts.push_back(cur);
            cur.clear();
        }
        else
            cur.push_back(ch);
    }

    out = "/";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        out += parts[i];
        if (i + 1 < parts.size())
            out += "/";
    }
    return true;
}

RouterByteHandler::RouterByteHandler(const std::string& configPath)
: _cfg()
, _router(NULL)
{
    std::ifstream file(configPath.c_str());
    if (!file)
        throw std::runtime_error("Cannot open config file: " + configPath);

    std::stringstream buffer;
    buffer << file.rdbuf();

    Tokenizer tokenizer(buffer.str());
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(tokens);
    _cfg = parser.parse();

    _router = new Router(_cfg);
}

RouterByteHandler::~RouterByteHandler()
{
    delete _router;
    _router = NULL;
}

ByteReply RouterByteHandler::handleBytes(int acceptFd, const std::string& rawMessage)
{
    // NOTE: PollReactor will call tryStartCgi() first.
    // handleBytes() is now for non-CGI requests only.
    HTTPRequest req;
    int err = 400;
    int listenPort = get_listen_port(acceptFd);

    if (!http10::parseRequest(rawMessage, listenPort, req, err))
    {
        if (err == 505)
            return ByteReply(http10::makeError(505, "HTTP Version Not Supported"), true);
        if (err == 405)
            return ByteReply(http10::makeError(405, "Method Not Allowed"), true);
        if (err == 411)
            return ByteReply(http10::makeError(411, "Length Required"), true);
        return ByteReply(http10::makeError(err, "Bad Request"), true);
    }

    HTTPResponse res = _router->handle_route_Request(req);
    return ByteReply(http10::serializeClose(res), true);
}

CgiStartResult RouterByteHandler::tryStartCgi(int acceptFd, const std::string& rawMessage)
{
    CgiStartResult out;

    HTTPRequest req;
    int err = 400;
    int listenPort = get_listen_port(acceptFd);

    if (!http10::parseRequest(rawMessage, listenPort, req, err))
        return out; // not CGI path; normal error handling stays in handleBytes()

    // replicate Router routing normalization for CGI detection
    std::string decoded;
    if (!url_decode_path(req.uri, decoded))
        return out;

    std::string norm;
    if (!normalize_uri_path(decoded, norm))
        return out;

    const ServerConfig& srv = _router->find_server_config(req);
    const LocationConfig* loc = _router->find_location_config(norm, srv);
    if (!loc)
        return out;

    std::string fullpath = _router->final_path(srv, *loc, norm);

    if (!_router->is_cgi_request(*loc, fullpath))
        return out;

    out.isCgi = true;

    Router::CgiSpawn sp;
    if (!_router->spawn_cgi(req, fullpath, *loc, sp))
    {
        out.ok = false;
        out.errResponseBytes = http10::makeError(502, "Bad Gateway");
        out.closeAfterWrite = true;
        return out;
    }

    out.ok = true;
    out.pid = sp.pid;
    out.fdIn = sp.fdIn;
    out.fdOut = sp.fdOut;

    if (!req.body.empty())
        out.body.assign(&req.body[0], req.body.size());

    out.closeAfterWrite = true;
    return out;
}

CgiFinishResult RouterByteHandler::finishCgi(int acceptFd, int /*clientFd*/, const std::string& cgiStdout)
{
    CgiFinishResult r;
    (void)acceptFd;

    // We don't re-route; CGI output already corresponds to the request path.
    // parse CGI headers/body, serialize as HTTP/1.0 close.
    HTTPResponse res = _router->parse_cgi_response(cgiStdout);

    r.responseBytes = http10::serializeClose(res);
    r.closeAfterWrite = true;
    return r;
}
