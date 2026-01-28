#ifndef ROUTERBYTEHANDLER_HPP
#define ROUTERBYTEHANDLER_HPP

#include "sockets/IByteHandler.hpp"
#include "config_headers/Config.hpp"
#include <string>

class Router;

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
    virtual ByteReply handleBytes(int acceptFd, const std::string& rawMessage);
};

#endif
