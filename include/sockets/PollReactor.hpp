#ifndef POLLREACTOR_HPP
#define POLLREACTOR_HPP

#include "IByteHandler.hpp"
#include "ICgiHandler.hpp"
#include "NetChannel.hpp"

#include <vector>
#include <map>
#include <set>
#include <string>
#include <poll.h>

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

    // CGI fds events
    void onCgiOutReadable(int fd);
    void onCgiInWritable(int fd);

    void sweepTimeouts();
    void maybeFinalizeCgi(int clientFd);

    std::string::size_type findHdrEnd(const std::string& buf);

    bool parseFramingHeaders(const std::string& headerBlock,
                             bool& outChunked,
                             bool& outHasLen,
                             size_t& outLen);

    std::string minimalError(int code, const char* reason);

    void dispatchIfIdle(NetChannel& ch);

    void cleanupCgiForClient(NetChannel& ch);

    /* -------------------- ASYNC UPLOAD SUPPORT -------------------- */
    bool tryStartAsyncUpload(NetChannel& ch, std::string& msg);
    void pumpAsyncUploads();
    void cleanupUploadForClient(NetChannel& ch);
    /* -------------------------------------------------------------- */

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

    // maps CGI pipe fd -> client fd
    std::map<int, int> _cgiOutToClient;
    std::map<int, int> _cgiInToClient;

    IByteHandler* _handler;
};

#endif
