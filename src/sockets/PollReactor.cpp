#include "../../include/sockets/PollReactor.hpp"
#include "../../include/sockets/NetUtil.hpp"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

static void setCloExec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags >= 0)
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

static bool is_retryable_errno(int e)
{
    return (e == EAGAIN || e == EWOULDBLOCK || e == EINTR);
}

static bool socket_has_fatal_error(int fd)
{
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        return true;
    return (err != 0);
}

PollReactor::PollReactor(const std::vector<int>& ports,
                         int backlog,
                         int idleTimeoutSec,
                         int headerTimeoutSec,
                         int bodyTimeoutSec,
                         size_t maxHeaderBytes,
                         size_t maxBodyBytes,
                         IByteHandler* handler)
: _listenSockets()
, _backlog(backlog)
, _idleTimeoutSec(idleTimeoutSec)
, _headerTimeoutSec(headerTimeoutSec)
, _bodyTimeoutSec(bodyTimeoutSec)
, _maxHeaderBytes(maxHeaderBytes)
, _maxBodyBytes(maxBodyBytes)
, _pollSet()
, _channels()
, _toDrop()
, _cgiOutToClient()
, _cgiInToClient()
, _handler(handler)
{
    if (!_handler)
        throw std::runtime_error("PollReactor: handler is null");

    initListeners(ports);

    for (size_t i = 0; i < _listenSockets.size(); ++i)
        addPollItem(_listenSockets[i], POLLIN);
}

PollReactor::~PollReactor()
{
    for (std::map<int, NetChannel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        cleanupCgiForClient(it->second);
        closeFd(it->first);
    }
    _channels.clear();

    for (std::map<int,int>::iterator it = _cgiOutToClient.begin(); it != _cgiOutToClient.end(); ++it)
        closeFd(it->first);
    for (std::map<int,int>::iterator it = _cgiInToClient.begin(); it != _cgiInToClient.end(); ++it)
        closeFd(it->first);

    _cgiOutToClient.clear();
    _cgiInToClient.clear();

    for (size_t i = 0; i < _listenSockets.size(); ++i)
        closeFd(_listenSockets[i]);
    _listenSockets.clear();
}

void PollReactor::initListeners(const std::vector<int>& ports)
{
    if (ports.empty())
        throw std::runtime_error("No ports provided");

    for (size_t i = 0; i < ports.size(); ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            throw std::runtime_error("socket() failed");
        setCloExec(fd);

        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            closeFd(fd);
            throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<unsigned short>(ports[i]));

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            closeFd(fd);
            throw std::runtime_error("bind() failed (port in use?)");
        }

        if (listen(fd, _backlog) < 0)
        {
            closeFd(fd);
            throw std::runtime_error("listen() failed");
        }

        if (makeNonBlocking(fd) < 0)
        {
            closeFd(fd);
            throw std::runtime_error("fcntl(O_NONBLOCK) failed");
        }

        _listenSockets.push_back(fd);
        std::cout << "Listening on port " << ports[i] << "\n";
    }
}

void PollReactor::addPollItem(int fd, short events)
{
    pollfd p;
    p.fd = fd;
    p.events = events;
    p.revents = 0;
    _pollSet.push_back(p);
}

void PollReactor::setPollMask(int fd, short events)
{
    for (size_t i = 0; i < _pollSet.size(); ++i)
    {
        if (_pollSet[i].fd == fd)
        {
            _pollSet[i].events = events;
            return;
        }
    }
}

void PollReactor::removePollItem(int fd)
{
    for (size_t i = 0; i < _pollSet.size(); ++i)
    {
        if (_pollSet[i].fd == fd)
        {
            _pollSet.erase(_pollSet.begin() + i);
            return;
        }
    }
}

bool PollReactor::isListener(int fd) const
{
    for (size_t i = 0; i < _listenSockets.size(); ++i)
        if (_listenSockets[i] == fd)
            return true;
    return false;
}

void PollReactor::markDrop(int fd)
{
    if (fd >= 0 && !isListener(fd))
        _toDrop.insert(fd);
}

void PollReactor::cleanupCgiForClient(NetChannel& ch)
{
    CgiSession& cg = ch.cgi();
    if (!cg.active)
        return;

    // remove poll items for CGI fds
    if (cg.fdOut >= 0)
    {
        removePollItem(cg.fdOut);
        _cgiOutToClient.erase(cg.fdOut);
        closeFd(cg.fdOut);
        cg.fdOut = -1;
    }
    if (cg.fdIn >= 0)
    {
        removePollItem(cg.fdIn);
        _cgiInToClient.erase(cg.fdIn);
        closeFd(cg.fdIn);
        cg.fdIn = -1;
    }

    if (cg.pid > 0)
    {
        kill(cg.pid, SIGKILL);
        waitpid(cg.pid, NULL, 0);
        cg.pid = -1;
    }
    cg.active = false;
}

void PollReactor::flushDrops()
{
    for (std::set<int>::iterator it = _toDrop.begin(); it != _toDrop.end(); ++it)
    {
        int fd = *it;

        std::map<int, NetChannel>::iterator chIt = _channels.find(fd);
        if (chIt != _channels.end())
        {
            cleanupCgiForClient(chIt->second);
        }

        removePollItem(fd);
        _channels.erase(fd);
        closeFd(fd);
    }
    _toDrop.clear();
}

void PollReactor::acceptBurst(int listenFd)
{
    while (true)
    {
        int clientFd = accept(listenFd, 0, 0);
        if (clientFd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            ::perror("accept");
            break;
        }

        setCloExec(clientFd);
        if (makeNonBlocking(clientFd) < 0)
        {
            closeFd(clientFd);
            continue;
        }

        _channels.insert(std::make_pair(clientFd, NetChannel(clientFd, listenFd)));

        NetChannel& ch = _channels[clientFd];
        ch.setPhase(PHASE_RECV_HEADERS);
        ch.markSeen();

        addPollItem(clientFd, POLLIN);
    }
}

std::string::size_type PollReactor::findHdrEnd(const std::string& buf)
{
    return buf.find("\r\n\r\n");
}

static std::string asciiLower(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(r[i]);
        if (c >= 'A' && c <= 'Z')
            r[i] = static_cast<char>(c - 'A' + 'a');
    }
    return r;
}

static bool parseDecSizeT(const std::string& s, size_t& out)
{
    if (s.empty()) return false;
    size_t v = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < '0' || c > '9')
            return false;
        size_t nv = v * 10 + (c - '0');
        if (nv < v) return false;
        v = nv;
    }
    out = v;
    return true;
}

bool PollReactor::parseFramingHeaders(const std::string& headerBlock,
                                      bool& outChunked,
                                      bool& outHasLen,
                                      size_t& outLen)
{
    outChunked = false;
    outHasLen = false;
    outLen = 0;

    size_t pos = 0;
    while (pos < headerBlock.size())
    {
        size_t eol = headerBlock.find("\r\n", pos);
        if (eol == std::string::npos)
            break;
        std::string line = headerBlock.substr(pos, eol - pos);
        pos = eol + 2;

        if (line.find(':') == std::string::npos)
            continue;

        std::string low = asciiLower(line);

        if (low.find("transfer-encoding:") == 0)
        {
            if (low.find("chunked") != std::string::npos)
                outChunked = true;
        }

        if (low.find("content-length:") == 0)
        {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                size_t i = colon + 1;
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
                std::string num = line.substr(i);
                size_t v = 0;
                if (!parseDecSizeT(num, v))
                    return false;
                outHasLen = true;
                outLen = v;
            }
        }
    }

    // your server refuses chunked (by design)
    if (outChunked)
        return false;

    return true;
}

void PollReactor::dispatchIfIdle(NetChannel& ch)
{
    if (ch.inFlight())
        return;
    if (ch.phase() == PHASE_SEND || !ch.txBuffer().empty())
        return;
    if (!ch.hasReadyMsg())
        return;

    std::string msg = ch.popReadyMsg();

    // If handler supports async CGI: try start CGI first
    ICgiHandler* cgiH = dynamic_cast<ICgiHandler*>(_handler);
    if (cgiH)
    {
        CgiStartResult st = cgiH->tryStartCgi(ch.acceptFd(), msg);
        if (st.isCgi)
        {
            if (!st.ok)
            {
                ch.txBuffer() = st.errResponseBytes;
                ch.setCloseOnDone(true);
                ch.setPhase(PHASE_SEND);
                setPollMask(ch.sockFd(), POLLIN | POLLOUT);
                return;
            }

            // Setup CGI session
            CgiSession& cg = ch.cgi();
            cg.active = true;
            cg.pid = st.pid;
            cg.fdIn = st.fdIn;
            cg.fdOut = st.fdOut;
            cg.inBody = st.body;
            cg.inOff = 0;
            cg.outBuf.clear();
            cg.startTs = std::time(NULL);
            cg.timeoutSec = 30;

            _cgiOutToClient[cg.fdOut] = ch.sockFd();
            _cgiInToClient[cg.fdIn] = ch.sockFd();

            addPollItem(cg.fdOut, POLLIN | POLLHUP);
            if (!cg.inBody.empty())
                addPollItem(cg.fdIn, POLLOUT);
            else
            {
                // nothing to write: close stdin immediately
                removePollItem(cg.fdIn);
                _cgiInToClient.erase(cg.fdIn);
                closeFd(cg.fdIn);
                cg.fdIn = -1;
            }

            ch.setInFlight(true);
            // keep client in POLLIN to detect hangup; do not read new requests while CGI in-flight
            setPollMask(ch.sockFd(), POLLIN);
            return;
        }
    }

    // Normal non-CGI: handle synchronously (fast)
    ch.setInFlight(true);
    ByteReply rep = _handler->handleBytes(ch.acceptFd(), msg);
    ch.setInFlight(false);

    ch.txBuffer() = rep.bytes;
    ch.setCloseOnDone(rep.closeAfterWrite);
    ch.setPhase(PHASE_SEND);
    setPollMask(ch.sockFd(), POLLIN | POLLOUT);
}

std::string PollReactor::minimalError(int code, const char* reason)
{
    std::string body = reason;
    body += "\n";

    char line[128];
    ::snprintf(line, sizeof(line), "HTTP/1.0 %d %s\r\n", code, reason);

    char clen[128];
    ::snprintf(clen, sizeof(clen), "Content-Length: %lu\r\n", (unsigned long)body.size());

    std::string r;
    r += line;
    r += "Content-Type: text/plain\r\n";
    r += clen;
    r += "Connection: close\r\n";
    r += "\r\n";
    r += body;
    return r;
}

void PollReactor::onReadable(int fd)
{
    std::map<int, NetChannel>::iterator it = _channels.find(fd);
    if (it == _channels.end())
        return;

    NetChannel& ch = it->second;

    // if CGI is running, we do not accept pipelined requests; only detect disconnect
    if (ch.cgi().active)
    {
        char tmp[1];
        ssize_t n = recv(fd, tmp, sizeof(tmp), MSG_PEEK);
        if (n == 0)
            markDrop(fd);
        return;
    }

    if (!(ch.phase() == PHASE_RECV_HEADERS || ch.phase() == PHASE_RECV_BODY))
        return;

    char buf[4096];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n > 0)
    {
        ch.rxBuffer().append(buf, n);
        ch.markSeen();

        if (ch.phase() == PHASE_RECV_HEADERS && ch.rxBuffer().size() > _maxHeaderBytes)
        {
            ch.txBuffer() = minimalError(431, "Request Header Fields Too Large");
            ch.setCloseOnDone(true);
            ch.setPhase(PHASE_SEND);
            setPollMask(fd, POLLIN | POLLOUT);
            return;
        }

        while (true)
        {
            if (ch.phase() == PHASE_RECV_HEADERS)
            {
                std::string::size_type he = findHdrEnd(ch.rxBuffer());
                if (he == std::string::npos)
                    break;

                ch.setHdrEnd(static_cast<size_t>(he));
                std::string headerBlock = ch.rxBuffer().substr(0, he + 2);

                bool chunked = false;
                bool hasLen = false;
                size_t len = 0;
                if (!parseFramingHeaders(headerBlock, chunked, hasLen, len))
                {
                    ch.txBuffer() = minimalError(400, "Bad Request");
                    ch.setCloseOnDone(true);
                    ch.setPhase(PHASE_SEND);
                    setPollMask(fd, POLLIN | POLLOUT);
                    return;
                }

                ch.setChunked(chunked);
                ch.setHasLen(hasLen);
                ch.setLen(len);

                if (chunked || (hasLen && len > 0))
                    ch.setPhase(PHASE_RECV_BODY);
                else
                {
                    std::string full = ch.rxBuffer().substr(0, he + 4);
                    ch.rxBuffer().erase(0, he + 4);

                    ch.resetFraming();
                    ch.pushReadyMsg(full);
                    continue;
                }
            }

            if (ch.phase() == PHASE_RECV_BODY)
            {
                size_t he = ch.hdrEnd();
                size_t bodyStart = he + 4;

                if (ch.hasLen() && _maxBodyBytes != 0 && ch.len() > _maxBodyBytes)
                {
                    ch.txBuffer() = minimalError(413, "Payload Too Large");
                    ch.setCloseOnDone(true);
                    ch.setPhase(PHASE_SEND);
                    setPollMask(fd, POLLIN | POLLOUT);
                    return;
                }

                size_t need = bodyStart + ch.len();
                if (ch.rxBuffer().size() < need)
                    break;
                if (ch.rxBuffer().size() > need)
                {
                    ch.txBuffer() = minimalError(400, "Bad Request");
                    ch.setCloseOnDone(true);
                    ch.setPhase(PHASE_SEND);
                    setPollMask(fd, POLLIN | POLLOUT);
                    return;
                }

                std::string full = ch.rxBuffer().substr(0, need);
                ch.rxBuffer().erase(0, need);

                ch.resetFraming();
                ch.setPhase(PHASE_RECV_HEADERS);
                ch.pushReadyMsg(full);
                continue;
            }

            break;
        }

        dispatchIfIdle(ch);
        return;
    }

    if (n == 0)
    {
        markDrop(fd);
        return;
    }

    const int e = errno;
    if (is_retryable_errno(e))
        return;

    markDrop(fd);
}

void PollReactor::onWritable(int fd)
{
    std::map<int, NetChannel>::iterator it = _channels.find(fd);
    if (it == _channels.end())
        return;

    NetChannel& ch = it->second;

    if (ch.phase() != PHASE_SEND)
        return;

    if (!ch.txBuffer().empty())
    {
        const std::string& out = ch.txBuffer();
        ssize_t n = send(fd, out.data(), out.size(), 0);

        if (n > 0)
        {
            ch.txBuffer().erase(0, n);
            ch.markSeen();
        }
        else if (n < 0)
        {
            if (socket_has_fatal_error(fd))
                markDrop(fd);
            return;
        }
    }

    if (ch.txBuffer().empty())
    {
        if (ch.closeOnDone())
        {
            ch.setPhase(PHASE_SHUTDOWN);
            markDrop(fd);
            return;
        }

        ch.setPhase(PHASE_RECV_HEADERS);
        setPollMask(fd, POLLIN);

        dispatchIfIdle(ch);
    }
}

void PollReactor::onCgiInWritable(int fd)
{
    std::map<int,int>::iterator it = _cgiInToClient.find(fd);
    if (it == _cgiInToClient.end())
        return;

    int clientFd = it->second;
    std::map<int, NetChannel>::iterator chIt = _channels.find(clientFd);
    if (chIt == _channels.end())
        return;

    NetChannel& ch = chIt->second;
    CgiSession& cg = ch.cgi();
    if (!cg.active || cg.fdIn != fd)
        return;

    if (cg.inOff >= cg.inBody.size())
    {
        // Finished writing body: close stdin to CGI
        removePollItem(fd);
        _cgiInToClient.erase(fd);
        closeFd(fd);
        cg.fdIn = -1;
        return;
    }

    const char* data = cg.inBody.data() + cg.inOff;
    size_t      left = cg.inBody.size() - cg.inOff;

    ssize_t n = write(fd, data, left);
    if (n > 0)
    {
        cg.inOff += (size_t)n;
        if (cg.inOff >= cg.inBody.size())
        {
            // Done -> close stdin
            removePollItem(fd);
            _cgiInToClient.erase(fd);
            closeFd(fd);
            cg.fdIn = -1;
        }
        return;
    }

    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return;

        // Hard failure writing to CGI stdin
        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(502, "Bad Gateway");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }
}

static bool cgiTimedOut(const CgiSession& cg)
{
    if (!cg.active) return false;
    if (cg.timeoutSec <= 0) return false;
    std::time_t now = std::time(NULL);
    return (now - cg.startTs) >= cg.timeoutSec;
}

void PollReactor::maybeFinalizeCgi(int clientFd)
{
    std::map<int, NetChannel>::iterator it = _channels.find(clientFd);
    if (it == _channels.end())
        return;

    NetChannel& ch = it->second;
    CgiSession& cg = ch.cgi();
    if (!cg.active)
        return;

    // timeout handling
    if (cgiTimedOut(cg))
    {
        // Kill CGI and reply 504
        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(504, "Gateway Timeout");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }

    // check if child exited (non-blocking)
    bool exited = false;
    int status = 0;
    if (cg.pid > 0)
    {
        pid_t w = waitpid(cg.pid, &status, WNOHANG);
        if (w == cg.pid)
            exited = true;
    }
    else
        exited = true;

    // We finalize only when:
    // - stdout fd is closed (cg.fdOut == -1)
    // - stdin is closed or not needed (cg.fdIn == -1)
    // - child exited
    if (!(exited && cg.fdOut == -1 && cg.fdIn == -1))
        return;

    // Build response using handler
    ICgiHandler* cgiH = dynamic_cast<ICgiHandler*>(_handler);
    if (!cgiH)
    {
        // Should never happen if we started CGI via ICgiHandler
        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(500, "Internal Server Error");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }

    CgiFinishResult fin = cgiH->finishCgi(ch.acceptFd(), ch.sockFd(), cg.outBuf);

    // cleanup CGI state (already done fds, but ensure it)
    cg.active = false;
    cg.pid = -1;
    cg.inBody.clear();
    cg.inOff = 0;

    ch.setInFlight(false);
    ch.txBuffer() = fin.responseBytes;
    ch.setCloseOnDone(fin.closeAfterWrite);
    ch.setPhase(PHASE_SEND);
    setPollMask(ch.sockFd(), POLLIN | POLLOUT);
}

void PollReactor::onCgiOutReadable(int fd)
{
    std::map<int,int>::iterator it = _cgiOutToClient.find(fd);
    if (it == _cgiOutToClient.end())
        return;

    int clientFd = it->second;
    std::map<int, NetChannel>::iterator chIt = _channels.find(clientFd);
    if (chIt == _channels.end())
    {
        // no client anymore => just close fd
        removePollItem(fd);
        _cgiOutToClient.erase(fd);
        closeFd(fd);
        return;
    }

    NetChannel& ch = chIt->second;
    CgiSession& cg = ch.cgi();
    if (!cg.active || cg.fdOut != fd)
        return;

    char buf[8192];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0)
    {
        cg.outBuf.append(buf, buf + n);
        // keep reading later
        return;
    }

    if (n == 0)
    {
        // EOF: close stdout
        removePollItem(fd);
        _cgiOutToClient.erase(fd);
        closeFd(fd);
        cg.fdOut = -1;

        // maybe we can finish now
        maybeFinalizeCgi(clientFd);
        return;
    }

    // n < 0
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return;

    // Hard failure reading CGI output
    cleanupCgiForClient(ch);
    ch.setInFlight(false);

    ch.txBuffer() = minimalError(502, "Bad Gateway");
    ch.setCloseOnDone(true);
    ch.setPhase(PHASE_SEND);
    setPollMask(ch.sockFd(), POLLIN | POLLOUT);
}

void PollReactor::onPollEvent(size_t idx)
{
    const int fd = _pollSet[idx].fd;
    const short re = _pollSet[idx].revents;

    // CGI fds first
    if (_cgiOutToClient.find(fd) != _cgiOutToClient.end())
    {
        if (re & (POLLERR | POLLNVAL | POLLHUP | POLLIN))
            onCgiOutReadable(fd);
        return;
    }
    if (_cgiInToClient.find(fd) != _cgiInToClient.end())
    {
        if (re & (POLLERR | POLLNVAL | POLLHUP))
        {
            // treat as write failure -> cleanup that client CGI
            int clientFd = _cgiInToClient[fd];
            std::map<int, NetChannel>::iterator chIt = _channels.find(clientFd);
            if (chIt != _channels.end())
            {
                NetChannel& ch = chIt->second;
                cleanupCgiForClient(ch);
                ch.setInFlight(false);

                ch.txBuffer() = minimalError(502, "Bad Gateway");
                ch.setCloseOnDone(true);
                ch.setPhase(PHASE_SEND);
                setPollMask(ch.sockFd(), POLLIN | POLLOUT);
            }
            return;
        }
        if (re & POLLOUT)
            onCgiInWritable(fd);
        return;
    }

    // Normal sockets
    if (re & (POLLERR | POLLNVAL))
    {
        if (!isListener(fd))
            markDrop(fd);
        return;
    }

    const bool hup = (re & POLLHUP) != 0;

    if (hup && !isListener(fd))
    {
        std::map<int, NetChannel>::iterator it = _channels.find(fd);
        if (it != _channels.end())
        {
            NetChannel& ch = it->second;
            ch.setCloseOnDone(true);

            if (ch.phase() == PHASE_SEND || !ch.txBuffer().empty())
                setPollMask(fd, POLLOUT);
            else
                setPollMask(fd, 0);
        }
        return;
    }

    if (re & POLLIN)
    {
        if (isListener(fd))
            acceptBurst(fd);
        else
            onReadable(fd);
    }

    if (re & POLLOUT)
    {
        if (!isListener(fd))
            onWritable(fd);
    }
}

void PollReactor::sweepTimeouts()
{
    const std::time_t now = std::time(NULL);

    for (std::map<int, NetChannel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        int fd = it->first;
        NetChannel& ch = it->second;

        // CGI timeout
        if (ch.cgi().active)
        {
            if (cgiTimedOut(ch.cgi()))
            {
                cleanupCgiForClient(ch);
                ch.setInFlight(false);

                ch.txBuffer() = minimalError(504, "Gateway Timeout");
                ch.setCloseOnDone(true);
                ch.setPhase(PHASE_SEND);
                setPollMask(ch.sockFd(), POLLIN | POLLOUT);
            }
            continue;
        }

        if (_idleTimeoutSec > 0 && (now - ch.lastSeen()) > _idleTimeoutSec)
        {
            markDrop(fd);
            continue;
        }
        if (ch.inFlight())
            continue;

        if (ch.phase() == PHASE_RECV_HEADERS && _headerTimeoutSec > 0 &&
            (now - ch.phaseSince()) > _headerTimeoutSec)
        {
            markDrop(fd);
            continue;
        }

        if (ch.phase() == PHASE_RECV_BODY && _bodyTimeoutSec > 0 &&
            (now - ch.phaseSince()) > _bodyTimeoutSec)
        {
            markDrop(fd);
            continue;
        }
    }
}

void PollReactor::tickOnce()
{
    if (_pollSet.empty())
        return;

    int ret = poll(&_pollSet[0], _pollSet.size(), 1000);
    if (ret < 0)
    {
        if (errno == EINTR)
            return;
        ::perror("poll");
        return;
    }

    if (ret > 0)
    {
        for (size_t i = 0; i < _pollSet.size(); ++i)
        {
            if (_pollSet[i].revents == 0)
                continue;
            onPollEvent(i);
        }
    }

    // After polling, finalize any CGI that completed without more fd events
    // (for example: child exits after stdout already closed)
    for (std::map<int, NetChannel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        NetChannel& ch = it->second;
        if (ch.cgi().active)
            maybeFinalizeCgi(it->first);
    }

    flushDrops();
    sweepTimeouts();
    flushDrops();
}