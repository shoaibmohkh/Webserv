#ifndef IBYTEHANDLER_HPP
#define IBYTEHANDLER_HPP

#include <string>

struct ByteReply {
    std::string bytes;
    bool        closeAfterWrite;

    ByteReply() : bytes(), closeAfterWrite(true) {}
    ByteReply(const std::string& b, bool c) : bytes(b), closeAfterWrite(c) {}
};

class IByteHandler
{
public:
    virtual ~IByteHandler() {}
    // acceptFd: which listener accepted it (Partner 2 can map to server config)
    // sockFd: for advanced logging if desired
    virtual ByteReply handleBytes(int acceptFd, int sockFd, const std::string& rawMessage) = 0;
};

#endif
