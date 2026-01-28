/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RouterByteHandler.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:17:45 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/01/28 11:17:56 by sal-kawa         ###   ########.fr       */
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
