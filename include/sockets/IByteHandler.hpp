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
    virtual ByteReply handleBytes(int acceptFd, const std::string& rawMessage) = 0;
};

#endif
