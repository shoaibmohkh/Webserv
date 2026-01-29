#include "../../include/sockets/NetChannel.hpp"

NetChannel::NetChannel()
: _sockFd(-1)
, _acceptFd(-1)
, _rx()
, _tx()
, _lastSeen(0)
, _phaseSince(0)
, _phase(PHASE_RECV_HEADERS)
, _hdrEnd(0)
, _hdrReady(false)
, _isChunked(false)
, _hasLen(false)
, _len(0)
, _readyMsgs()
, _closeOnDone(true)
, _inFlight(false)
, _cgi()
{
    markSeen();
    markPhaseSince();
}

NetChannel::NetChannel(int sockFd, int acceptFd)
: _sockFd(sockFd)
, _acceptFd(acceptFd)
, _rx()
, _tx()
, _lastSeen(0)
, _phaseSince(0)
, _phase(PHASE_RECV_HEADERS)
, _hdrEnd(0)
, _hdrReady(false)
, _isChunked(false)
, _hasLen(false)
, _len(0)
, _readyMsgs()
, _closeOnDone(true)
, _inFlight(false)
, _cgi()
{
    markSeen();
    markPhaseSince();
}

int NetChannel::sockFd() const { return _sockFd; }
int NetChannel::acceptFd() const { return _acceptFd; }

std::string& NetChannel::rxBuffer() { return _rx; }
std::string& NetChannel::txBuffer() { return _tx; }

std::time_t NetChannel::lastSeen() const { return _lastSeen; }
void NetChannel::markSeen() { _lastSeen = std::time(NULL); }

std::time_t NetChannel::phaseSince() const { return _phaseSince; }
void NetChannel::markPhaseSince() { _phaseSince = std::time(NULL); }

IoPhase NetChannel::phase() const { return _phase; }
void NetChannel::setPhase(IoPhase p) { _phase = p; markPhaseSince(); }

void NetChannel::resetFraming()
{
    _hdrEnd = 0;
    _hdrReady = false;
    _isChunked = false;
    _hasLen = false;
    _len = 0;
}

bool NetChannel::hdrReady() const { return _hdrReady; }
size_t NetChannel::hdrEnd() const { return _hdrEnd; }

void NetChannel::setHdrEnd(size_t pos)
{
    _hdrEnd = pos;
    _hdrReady = true;
}

bool NetChannel::isChunked() const { return _isChunked; }
void NetChannel::setChunked(bool v) { _isChunked = v; }

bool NetChannel::hasLen() const { return _hasLen; }
void NetChannel::setHasLen(bool v) { _hasLen = v; }

size_t NetChannel::len() const { return _len; }
void NetChannel::setLen(size_t n) { _len = n; }

bool NetChannel::hasReadyMsg() const { return !_readyMsgs.empty(); }

std::string NetChannel::popReadyMsg()
{
    if (_readyMsgs.empty())
        return std::string();
    std::string r = _readyMsgs.front();
    _readyMsgs.pop_front();
    return r;
}

void NetChannel::pushReadyMsg(const std::string& msg)
{
    _readyMsgs.push_back(msg);
}

bool NetChannel::closeOnDone() const { return _closeOnDone; }
void NetChannel::setCloseOnDone(bool v) { _closeOnDone = v; }

bool NetChannel::inFlight() const { return _inFlight; }
void NetChannel::setInFlight(bool v) { _inFlight = v; markPhaseSince(); }

CgiSession& NetChannel::cgi() { return _cgi; }
