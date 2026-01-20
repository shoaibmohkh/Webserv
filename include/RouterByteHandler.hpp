#ifndef ROUTERBYTEHANDLER_HPP
#define ROUTERBYTEHANDLER_HPP

#include "sockets/IByteHandler.hpp"

// Partner 3 includes
#include "config_headers/Tokenizer.hpp"
#include "config_headers/Parser.hpp"
#include "Router_headers/Router.hpp"

#include <string>

class RouterByteHandler : public IByteHandler
{
private:
    Config  _cfg;
    Router* _router;

    RouterByteHandler(const RouterByteHandler&);
    RouterByteHandler& operator=(const RouterByteHandler&);

public:
    RouterByteHandler(const std::string& configPath);
    virtual ~RouterByteHandler();

    virtual ByteReply handleBytes(int acceptFd, int sockFd, const std::string& rawMessage);
};

#endif
