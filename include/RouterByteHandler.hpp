#ifndef ROUTERBYTEHANDLER_HPP
#define ROUTERBYTEHANDLER_HPP

#include "sockets/IByteHandler.hpp"
#include "sockets/ICgiHandler.hpp"
#include "config_headers/Config.hpp"
#include <string>

class Router;

class RouterByteHandler : public IByteHandler, public ICgiHandler
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

    virtual CgiStartResult tryStartCgi(int acceptFd, const std::string& rawMessage);
    virtual CgiFinishResult finishCgi(int acceptFd, int clientFd, const std::string& cgiStdout);
    bool planUploadFd(int acceptFd,
                  const std::string& uri,
                  const std::string& mpFilename,
                  int& outFd,
                  std::string& outErrBytes);
};

#endif
