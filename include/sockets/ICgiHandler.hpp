#ifndef ICGIHANDLER_HPP
#define ICGIHANDLER_HPP

#include <string>
#include <sys/types.h>

struct CgiStartResult
{
    bool   isCgi;   // request is CGI or not
    bool   ok;      // if isCgi, did spawn succeed?
    pid_t  pid;
    int    fdIn;    // parent writes request body to CGI stdin
    int    fdOut;   // parent reads CGI stdout
    std::string body; // request body to feed
    std::string errResponseBytes; // if ok==false, send this response
    bool   closeAfterWrite;

    CgiStartResult()
    : isCgi(false), ok(false), pid(-1), fdIn(-1), fdOut(-1),
      body(), errResponseBytes(), closeAfterWrite(true)
    {}
};

struct CgiFinishResult
{
    std::string responseBytes;
    bool        closeAfterWrite;
    CgiFinishResult() : responseBytes(), closeAfterWrite(true) {}
};

class ICgiHandler
{
public:
    virtual ~ICgiHandler() {}

    virtual CgiStartResult tryStartCgi(int acceptFd, const std::string& rawMessage) = 0;

    virtual CgiFinishResult finishCgi(int acceptFd,
                                      int clientFd,
                                      const std::string& cgiStdout) = 0;
};

#endif
