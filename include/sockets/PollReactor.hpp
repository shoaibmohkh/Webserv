#ifndef POLLREACTOR_HPP
#define POLLREACTOR_HPP

#include "IByteHandler.hpp"
#include "NetChannel.hpp"

#include <vector>
#include <map>
#include <set>
#include <string>

#include <poll.h>
#include <pthread.h>

class PollReactor
{
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
    void pushDone(int clientFd, const std::string& bytes, bool closeAfterWrite);

private:
    void initListeners(const std::vector<int>& ports);

    void addPollItem(int fd, short events);
    void setPollMask(int fd, short events);
    void removePollItem(int fd);

    bool isListener(int fd) const;

    void markDrop(int fd);
    void flushDrops();

    void acceptBurst(int listenFd);

    void onPollEvent(size_t idx);
    void onReadable(int fd);
    void onWritable(int fd);

    void sweepTimeouts();
    std::string::size_type findHdrEnd(const std::string& buf);

    bool parseFramingHeaders(const std::string& headerBlock,
                             bool& outChunked,
                             bool& outHasLen,
                             size_t& outLen);

    bool tryUnchunk(const std::string& rx,
                    size_t bodyStart,
                    size_t& outMsgEnd,
                    std::string& outBody);

    std::string buildNormalized(const std::string& headerBlock,
                                const std::string& body,
                                bool wasChunked);

    std::string minimalError(int code, const char* reason);

    void dispatchIfIdle(NetChannel& ch);
    struct DoneItem
    {
        int         fd;
        std::string bytes;
        bool        closeAfterWrite;
    };

    void drainDone();

private:
    std::vector<int> _listenSockets;
    int _backlog;

    int _idleTimeoutSec;
    int _headerTimeoutSec;
    int _bodyTimeoutSec;

    size_t _maxHeaderBytes;
    size_t _maxBodyBytes;

    std::vector<pollfd> _pollSet;
    std::map<int, NetChannel> _channels;
    std::set<int> _toDrop;

    IByteHandler* _handler;
    pthread_mutex_t _doneMutex;
    std::vector<DoneItem> _done;
};

#endif
