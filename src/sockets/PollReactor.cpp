#include "../../include/sockets/PollReactor.hpp"
#include "../../include/sockets/NetUtil.hpp"
#include "../../include/RouterByteHandler.hpp"

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

static size_t find_mem(const char* hay, size_t hayLen,
                       const char* needle, size_t needleLen,
                       size_t start)
{
    if (!hay || !needle || needleLen == 0 || hayLen < needleLen ||
        (hayLen >= needleLen && start > hayLen - needleLen))
        return (size_t)-1;

    for (size_t i = start; i + needleLen <= hayLen; ++i)
    {
        if (std::memcmp(hay + i, needle, needleLen) == 0)
            return i;
    }
    return (size_t)-1;
}

static std::string trimSpaces(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t')) a++;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}

static bool headerValueCI(const std::string& headerBlock,
                          const std::string& keyLower,
                          std::string& outVal)
{
    size_t pos = 0;
    while (pos < headerBlock.size())
    {
        size_t eol = headerBlock.find("\r\n", pos);
        if (eol == std::string::npos)
            break;

        std::string line = headerBlock.substr(pos, eol - pos);
        pos = eol + 2;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string k = line.substr(0, colon);
        k = asciiLower(k);

        if (k == keyLower)
        {
            std::string v = line.substr(colon + 1);
            outVal = trimSpaces(v);
            return true;
        }
    }
    return false;
}

static bool extractBoundary(const std::string& contentType, std::string& outBoundary)
{
    std::string low = asciiLower(contentType);

    if (low.find("multipart/form-data") == std::string::npos)
        return false;

    size_t bpos = low.find("boundary=");
    if (bpos == std::string::npos)
        return false;

    std::string b = contentType.substr(bpos + 9);
    b = trimSpaces(b);
    if (b.empty())
        return false;

    if (b.size() >= 2 && b[0] == '"' && b[b.size() - 1] == '"')
        b = b.substr(1, b.size() - 2);

    if (b.empty())
        return false;

    outBoundary = b;
    return true;
}

static bool parseFilenameFromDisposition(const std::string& disp, std::string& outName)
{
    size_t p = disp.find("filename=");
    if (p == std::string::npos)
        return false;

    p += 9;
    if (p >= disp.size())
        return false;

    if (disp[p] == '"')
    {
        size_t q = disp.find('"', p + 1);
        if (q == std::string::npos)
            return false;
        outName = disp.substr(p + 1, q - (p + 1));
        return !outName.empty();
    }

    size_t end = disp.find(';', p);
    if (end == std::string::npos)
        end = disp.size();
    outName = trimSpaces(disp.substr(p, end - p));
    return !outName.empty();
}

static bool cgiTimedOut(const CgiSession& cg)
{
    if (!cg.active) return false;
    if (cg.timeoutSec <= 0) return false;
    std::time_t now = std::time(NULL);
    return (now - cg.startTs) >= cg.timeoutSec;
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
        cleanupUploadForClient(it->second);
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

void PollReactor::cleanupUploadForClient(NetChannel& ch)
{
    NetChannel::UploadSession& up = ch.upload();
    if (!up.active)
        return;

    if (up.fd >= 0)
    {
        closeFd(up.fd);
        up.fd = -1;
    }
    up.active = false;
    up.raw.clear();
    up.dataStart = 0;
    up.dataEnd = 0;
    up.off = 0;
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
            cleanupUploadForClient(chIt->second);
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
        return false;

    return true;
}

std::string PollReactor::minimalError(int code, const char* reason)
{
    std::string body = reason;
    body += "\n";

    char line[128];
    ::snprintf(line, sizeof(line), "HTTP/1.0 %d %s\r\n", code, reason);

    char clen[128];
    ::snprintf(clen, sizeof(clen), "Content-Length: %lu\r\n",
               (unsigned long)body.size());

    std::string r;
    r += line;
    r += "Content-Type: text/plain\r\n";
    r += clen;
    r += "Connection: close\r\n";
    r += "\r\n";
    r += body;
    return r;
}

bool PollReactor::tryStartAsyncUpload(NetChannel& ch, std::string& msg)
{

    const size_t THRESH = 5 * 1024 * 1024; // 5MB

    size_t he = msg.find("\r\n\r\n");
    if (he == std::string::npos)
        return false;

    size_t sp1 = msg.find(' ');
    if (sp1 == std::string::npos) return false;
    std::string method = msg.substr(0, sp1);
    if (method != "POST")
        return false;

    size_t sp2 = msg.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return false;
    std::string uri = msg.substr(sp1 + 1, sp2 - (sp1 + 1));

    std::string headerBlock = msg.substr(0, he + 2);
    std::string cl;
    if (!headerValueCI(headerBlock, "content-length", cl))
        return false;

    size_t contentLen = 0;
    if (!parseDecSizeT(cl, contentLen))
        return false;

    if (contentLen < THRESH)
        return false;

    size_t dataStart = 0;
    size_t dataEnd = 0;
    std::string mpFilename;

    std::string ct;
    bool isMultipart = false;
    std::string boundary;

    if (headerValueCI(headerBlock, "content-type", ct) && extractBoundary(ct, boundary))
        isMultipart = true;

    if (!isMultipart)
    {
        dataStart = he + 4;
        dataEnd = msg.size();
    }
    else
    {
        const char* b = msg.data();
        size_t blen = msg.size();

        std::string delim = "--" + boundary;
        std::string nextDelim = "\r\n" + delim;

        size_t p0 = find_mem(b, blen, delim.c_str(), delim.size(), he + 4);
        if (p0 == (size_t)-1) return false;

        size_t p = find_mem(b, blen, "\r\n", 2, p0);
        if (p == (size_t)-1) return false;
        p += 2;

        size_t hdrEnd = find_mem(b, blen, "\r\n\r\n", 4, p);
        if (hdrEnd == (size_t)-1) return false;

        std::string partHeaders(b + p, hdrEnd - p);

        std::string cdLine;
        {
            std::string low = asciiLower(partHeaders);
            size_t cd = low.find("content-disposition:");
            if (cd != std::string::npos)
            {
                size_t eol = partHeaders.find("\r\n", cd);
                if (eol == std::string::npos) eol = partHeaders.size();
                cdLine = partHeaders.substr(cd, eol - cd);
            }
        }
        if (!cdLine.empty())
            parseFilenameFromDisposition(cdLine, mpFilename);

        dataStart = hdrEnd + 4;

        size_t p1 = find_mem(b, blen, nextDelim.c_str(), nextDelim.size(), dataStart);
        if (p1 == (size_t)-1) return false;

        dataEnd = p1;
        if (dataEnd < dataStart) return false;
    }

    RouterByteHandler* rb = dynamic_cast<RouterByteHandler*>(_handler);
    if (!rb)
        return false;

    int outFd = -1;
    std::string errBytes;
    if (!rb->planUploadFd(ch.acceptFd(), uri, mpFilename, outFd, errBytes))
        return false;

    if (outFd < 0)
    {
        ch.txBuffer() = errBytes.empty()
            ? minimalError(500, "Internal Server Error")
            : errBytes;
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return true;
    }

    NetChannel::UploadSession& up = ch.upload();
    up.active = true;
    up.fd = outFd;
    up.raw.swap(msg);
    up.dataStart = dataStart;
    up.dataEnd = dataEnd;
    up.off = 0;

    ch.setInFlight(true);

    setPollMask(ch.sockFd(), 0);
    return true;
}


void PollReactor::pumpAsyncUploads()
{
    const size_t CHUNK = 1024 * 1024;
    const int MAX_WRITES_PER_TICK = 10;

    for (std::map<int, NetChannel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        NetChannel& ch = it->second;
        NetChannel::UploadSession& up = ch.upload();
        if (!up.active)
            continue;

        size_t total = (up.dataEnd > up.dataStart) ? (up.dataEnd - up.dataStart) : 0;
        if (up.off >= total)
        {
            // done
            closeFd(up.fd);
            up.fd = -1;
            up.active = false;
            up.raw.clear();
            ch.setInFlight(false);

            std::string body = "201 Created: File uploaded successfully.\n";
            char head[256];
            ::snprintf(head, sizeof(head),
                       "HTTP/1.0 201 Created\r\n"
                       "Connection: close\r\n"
                       "Content-Length: %lu\r\n"
                       "Content-Type: text/plain\r\n"
                       "\r\n",
                       (unsigned long)body.size());

            ch.txBuffer() = std::string(head) + body;
            ch.setCloseOnDone(true);
            ch.setPhase(PHASE_SEND);
            setPollMask(ch.sockFd(), POLLIN | POLLOUT);
            continue;
        }

        int writes_this_tick = 0;
        while (writes_this_tick < MAX_WRITES_PER_TICK && up.off < total)
        {
            size_t left = total - up.off;
            size_t nwrite = (left > CHUNK) ? CHUNK : left;

            const char* data = up.raw.data() + up.dataStart + up.off;
            ssize_t n = write(up.fd, data, nwrite);

            if (n > 0)
            {
                up.off += (size_t)n;
                writes_this_tick++;
                
                
                continue;
            }

            if (n < 0)
            {
                int err = errno;
                
                if (err == EAGAIN || err == EWOULDBLOCK)
                {
                    break;
                }

                if (err == EINTR)
                {
                    continue;
                }

                // Real failure
                closeFd(up.fd);
                up.fd = -1;
                up.active = false;
                up.raw.clear();
                ch.setInFlight(false);

                ch.txBuffer() = minimalError(500, "Internal Server Error");
                ch.setCloseOnDone(true);
                ch.setPhase(PHASE_SEND);
                setPollMask(ch.sockFd(), POLLIN | POLLOUT);
                break;
            }

            break;
        }
    }
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

    // Try async CGI first
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
                removePollItem(cg.fdIn);
                _cgiInToClient.erase(cg.fdIn);
                closeFd(cg.fdIn);
                cg.fdIn = -1;
            }

            ch.setInFlight(true);
            setPollMask(ch.sockFd(), POLLIN);
            return;
        }
    }

    // Try async upload
    if (tryStartAsyncUpload(ch, msg))
        return;

    // Normal sync request
    ch.setInFlight(true);
    ByteReply rep = _handler->handleBytes(ch.acceptFd(), msg);
    ch.setInFlight(false);

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

     if (ch.inFlight() && ch.upload().active)
    {

        ch.setCloseOnDone(true);
        setPollMask(fd, 0);
        return;
    }
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
        if (ch.phase() == PHASE_RECV_BODY)
            ch.markPhaseSince();

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

                std::string full;
                full.swap(ch.rxBuffer()); // O(1)

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
        removePollItem(fd);
        _cgiInToClient.erase(fd);
        closeFd(fd);
        cg.fdIn = -1;
        return;
    }

    const char* data = cg.inBody.data() + cg.inOff;
    size_t left = cg.inBody.size() - cg.inOff;

    ssize_t n = write(fd, data, left);
    if (n > 0)
    {
        cg.inOff += (size_t)n;
        if (cg.inOff >= cg.inBody.size())
        {
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

        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(502, "Bad Gateway");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }
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

    if (cgiTimedOut(cg))
    {
        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(504, "Gateway Timeout");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }

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

    if (!(exited && cg.fdOut == -1 && cg.fdIn == -1))
        return;

    ICgiHandler* cgiH = dynamic_cast<ICgiHandler*>(_handler);
    if (!cgiH)
    {
        cleanupCgiForClient(ch);
        ch.setInFlight(false);

        ch.txBuffer() = minimalError(500, "Internal Server Error");
        ch.setCloseOnDone(true);
        ch.setPhase(PHASE_SEND);
        setPollMask(ch.sockFd(), POLLIN | POLLOUT);
        return;
    }

    CgiFinishResult fin = cgiH->finishCgi(ch.acceptFd(), ch.sockFd(), cg.outBuf);

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
        return;
    }

    if (n == 0)
    {
        removePollItem(fd);
        _cgiOutToClient.erase(fd);
        closeFd(fd);
        cg.fdOut = -1;

        maybeFinalizeCgi(clientFd);
        return;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return;

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
            
            if (ch.upload().active)
            {

                return;
            }
            
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

        if (ch.upload().active)
        {
            continue;
        }

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

    int ret = poll(&_pollSet[0], _pollSet.size(), 0);
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

    // finalize CGI possibly without more fd events
    for (std::map<int, NetChannel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        NetChannel& ch = it->second;
        if (ch.cgi().active)
            maybeFinalizeCgi(it->first);
    }

    pumpAsyncUploads();

    flushDrops();
    sweepTimeouts();
    flushDrops();
}
