#include "../../include/sockets/PollReactor.hpp"
#include "../../include/sockets/NetUtil.hpp"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
        closeFd(it->first);
    _channels.clear();

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

void PollReactor::flushDrops()
{
    for (std::set<int>::iterator it = _toDrop.begin(); it != _toDrop.end(); ++it)
    {
        int fd = *it;

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
            // accept is not read/write; errno checks here are fine
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept");
            break;
        }

        if (makeNonBlocking(clientFd) < 0)
        {
            closeFd(clientFd);
            continue;
        }

        _channels.insert(std::make_pair(clientFd, NetChannel(clientFd, listenFd)));
        addPollItem(clientFd, POLLIN); // start in read mode only
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

    if (outChunked)
    {
        outHasLen = false;
        outLen = 0;
    }
    return true;
}

static bool parseHexSizeStrict(const std::string& s, size_t& out)
{
    size_t end = s.find(';');
    std::string hex = (end == std::string::npos) ? s : s.substr(0, end);

    size_t a = 0;
    while (a < hex.size() && (hex[a] == ' ' || hex[a] == '\t')) a++;
    size_t b = hex.size();
    while (b > a && (hex[b-1] == ' ' || hex[b-1] == '\t')) b--;
    hex = hex.substr(a, b - a);

    if (hex.empty()) return false;

    size_t v = 0;
    for (size_t i = 0; i < hex.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(hex[i]);
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else return false;

        size_t nv = (v << 4) + static_cast<size_t>(digit);
        if (nv < v) return false;
        v = nv;
    }
    out = v;
    return true;
}

bool PollReactor::tryUnchunk(const std::string& rx,
                             size_t bodyStart,
                             size_t& outMsgEnd,
                             std::string& outBody)
{
    outBody.clear();
    size_t pos = bodyStart;

    while (true)
    {
        size_t lineEnd = rx.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            return false;

        std::string sizeLine = rx.substr(pos, lineEnd - pos);
        size_t chunkSize = 0;
        if (!parseHexSizeStrict(sizeLine, chunkSize))
            return false;

        pos = lineEnd + 2;

        if (rx.size() < pos + chunkSize + 2)
            return false;

        if (chunkSize > 0)
            outBody.append(rx, pos, chunkSize);

        pos += chunkSize;

        if (!(rx[pos] == '\r' && rx[pos + 1] == '\n'))
            return false;
        pos += 2;

        if (chunkSize == 0)
        {
            size_t trailerEnd = rx.find("\r\n\r\n", pos);
            if (trailerEnd == std::string::npos)
                return false;
            outMsgEnd = trailerEnd + 4;
            return true;
        }
    }
}

static std::string stripOneHeaderKey(const std::string& headerBlock, const std::string& keyLower)
{
    std::string out;
    size_t pos = 0;
    while (pos < headerBlock.size())
    {
        size_t eol = headerBlock.find("\r\n", pos);
        if (eol == std::string::npos)
            break;
        std::string line = headerBlock.substr(pos, eol - pos);
        pos = eol + 2;

        std::string low = asciiLower(line);
        if (low.find(keyLower) == 0)
            continue;

        out += line;
        out += "\r\n";
    }
    return out;
}

std::string PollReactor::buildNormalized(const std::string& headerBlock,
                                         const std::string& body,
                                         bool wasChunked)
{
    std::string headers = headerBlock;

    if (wasChunked)
    {
        headers = stripOneHeaderKey(headers, "transfer-encoding:");
        headers = stripOneHeaderKey(headers, "content-length:");

        headers += "Content-Length: ";
        char tmp[64];
        std::snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)body.size());
        headers += tmp;
        headers += "\r\n";
    }

    std::string out = headers + "\r\n" + body;
    return out;
}

std::string PollReactor::minimalError(int code, const char* reason)
{
    std::string body = reason;
    body += "\n";

    char line[128];
    std::snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", code, reason);

    char clen[128];
    std::snprintf(clen, sizeof(clen), "Content-Length: %lu\r\n", (unsigned long)body.size());

    std::string r;
    r += line;
    r += "Content-Type: text/plain\r\n";
    r += clen;
    r += "Connection: close\r\n";
    r += "\r\n";
    r += body;
    return r;
}

void PollReactor::dispatchIfIdle(NetChannel& ch)
{
    if (ch.phase() == PHASE_SEND || !ch.txBuffer().empty())
        return;

    if (!ch.hasReadyMsg())
        return;

    std::string msg = ch.popReadyMsg();
    ByteReply rep = _handler->handleBytes(ch.acceptFd(), ch.sockFd(), msg);

    ch.txBuffer() = rep.bytes;
    ch.setCloseOnDone(rep.closeAfterWrite);
    ch.setPhase(PHASE_SEND);

    setPollMask(ch.sockFd(), POLLIN | POLLOUT);
}

void PollReactor::onReadable(int fd)
{
    std::map<int, NetChannel>::iterator it = _channels.find(fd);
    if (it == _channels.end())
        return;

    NetChannel& ch = it->second;

    if (!(ch.phase() == PHASE_RECV_HEADERS || ch.phase() == PHASE_RECV_BODY))
        return;

    char buf[4096];

    // One recv per POLLIN event (subject-safe pattern)
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0)
    {
        ch.rxBuffer().append(buf, n);
        ch.markSeen();

        // Header size guard (slowloris / memory)
        if (ch.phase() == PHASE_RECV_HEADERS && ch.rxBuffer().size() > _maxHeaderBytes)
        {
            ch.txBuffer() = minimalError(431, "Request Header Fields Too Large");
            ch.setCloseOnDone(true);
            ch.setPhase(PHASE_SEND);
            setPollMask(fd, POLLIN | POLLOUT);
            return;
        }

        // Attempt to frame as many messages as possible
        while (true)
        {
            if (ch.phase() == PHASE_RECV_HEADERS)
            {
                std::string::size_type he = findHdrEnd(ch.rxBuffer());
                if (he == std::string::npos)
                    break;

                ch.setHdrEnd(static_cast<size_t>(he));

                // Extract header block excluding the final "\r\n\r\n"
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
                    // Full message is headers-only
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

                if (ch.hasLen() && ch.len() > _maxBodyBytes)
                {
                    ch.txBuffer() = minimalError(413, "Payload Too Large");
                    ch.setCloseOnDone(true);
                    ch.setPhase(PHASE_SEND);
                    setPollMask(fd, POLLIN | POLLOUT);
                    return;
                }

                if (ch.rxBuffer().size() > (_maxHeaderBytes + _maxBodyBytes + 8192))
                {
                    ch.txBuffer() = minimalError(413, "Payload Too Large");
                    ch.setCloseOnDone(true);
                    ch.setPhase(PHASE_SEND);
                    setPollMask(fd, POLLIN | POLLOUT);
                    return;
                }

                if (ch.isChunked())
                {
                    size_t msgEnd = 0;
                    std::string body;
                    bool ok = tryUnchunk(ch.rxBuffer(), bodyStart, msgEnd, body);
                    if (!ok)
                        break;

                    std::string headerBlock = ch.rxBuffer().substr(0, he + 2);
                    std::string full = buildNormalized(headerBlock, body, true);

                    ch.rxBuffer().erase(0, msgEnd);

                    ch.resetFraming();
                    ch.setPhase(PHASE_RECV_HEADERS);
                    ch.pushReadyMsg(full);
                    continue;
                }
                else
                {
                    size_t need = bodyStart + ch.len();
                    if (ch.rxBuffer().size() < need)
                        break;

                    std::string full = ch.rxBuffer().substr(0, need);
                    ch.rxBuffer().erase(0, need);

                    ch.resetFraming();
                    ch.setPhase(PHASE_RECV_HEADERS);
                    ch.pushReadyMsg(full);
                    continue;
                }
            }

            break;
        }

        dispatchIfIdle(ch);
    }
    else if (n == 0)
    {
        markDrop(fd);
    }
    else
    {
        // No errno-based branching here; SO_ERROR determines fatal vs retry-later
        if (socket_has_fatal_error(fd))
            markDrop(fd);
    }
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

void PollReactor::onPollEvent(size_t idx)
{
    const int fd = _pollSet[idx].fd;
    const short re = _pollSet[idx].revents;

    if (re & (POLLERR | POLLHUP | POLLNVAL))
    {
        if (!isListener(fd))
            markDrop(fd);
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

        if (_idleTimeoutSec > 0 && (now - ch.lastSeen()) > _idleTimeoutSec)
        {
            markDrop(fd);
            continue;
        }

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
        perror("poll");
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

    flushDrops();
    sweepTimeouts();
    flushDrops();
}
