#ifndef POLLREACTOR_HPP
#define POLLREACTOR_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <ctime>
#include <poll.h>
#include <cstdio>

#include "NetChannel.hpp"
#include "IByteHandler.hpp"

class PollReactor
{
private:
    std::vector<int>              _listenSockets;
    int                           _backlog;

    // Timeouts / limits
    int                           _idleTimeoutSec;
    int                           _headerTimeoutSec;
    int                           _bodyTimeoutSec;
    size_t                        _maxHeaderBytes;
    size_t                        _maxBodyBytes;

    std::vector<pollfd>           _pollSet;
    std::map<int, NetChannel>     _channels;
    std::set<int>                 _toDrop;

    IByteHandler*                 _handler;

    PollReactor(const PollReactor&);
    PollReactor& operator=(const PollReactor&);

private:
    void initListeners(const std::vector<int>& ports);

    void addPollItem(int fd, short events);
    void setPollMask(int fd, short events);
    void removePollItem(int fd);

    bool isListener(int fd) const;

    void acceptBurst(int listenFd);

    void onPollEvent(size_t idx);
    void onReadable(int fd);
    void onWritable(int fd);

    void markDrop(int fd);
    void flushDrops();

    void sweepTimeouts();

    // Framing
    static std::string::size_type findHdrEnd(const std::string& buf);

    // Minimal header parsing (only for message boundaries)
    static bool parseFramingHeaders(const std::string& headerBlock,
                                    bool& outChunked,
                                    bool& outHasLen,
                                    size_t& outLen);

    // Chunked parsing + unchunking into a normalized request bytes
    static bool tryUnchunk(const std::string& rx,
                           size_t bodyStart,
                           size_t& outMsgEnd,
                           std::string& outBody);

    // Build normalized message bytes (headers + CRLFCRLF + body) after unchunking
    static std::string buildNormalized(const std::string& headerBlock,
                                       const std::string& body,
                                       bool wasChunked);

    // Internal minimal error response (keeps server stable under bad input)
    static std::string minimalError(int code, const char* reason);

    // Dispatch framed messages (from queue) to handler and manage send state
    void dispatchIfIdle(NetChannel& ch);

public:
    PollReactor(const std::vector<int>& ports,
                int backlog,
                int idleTimeoutSec,
                int headerTimeoutSec,
                int bodyTimeoutSec,
                size_t maxHeaderBytes,
                size_t maxBodyBytes,
                IByteHandler* handler);

    ~PollReactor();

    void tickOnce();
};

#endif
