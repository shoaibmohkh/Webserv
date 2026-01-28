#ifndef NETCHANNEL_HPP
#define NETCHANNEL_HPP

#include <string>
#include <deque>
#include <ctime>

enum IoPhase
{
    PHASE_RECV_HEADERS = 0,
    PHASE_RECV_BODY    = 1,
    PHASE_SEND         = 2,
    PHASE_SHUTDOWN     = 3
};

class NetChannel
{
public:
    NetChannel();
    NetChannel(int sockFd, int acceptFd);

    int sockFd() const;
    int acceptFd() const;

    std::string& rxBuffer();
    std::string& txBuffer();

    std::time_t lastSeen() const;
    void markSeen();

    std::time_t phaseSince() const;
    void markPhaseSince();

    IoPhase phase() const;
    void setPhase(IoPhase p);

    void resetFraming();

    bool hdrReady() const;
    size_t hdrEnd() const;
    void setHdrEnd(size_t pos);

    bool isChunked() const;
    void setChunked(bool v);

    bool hasLen() const;
    void setHasLen(bool v);

    size_t len() const;
    void setLen(size_t n);

    bool hasReadyMsg() const;
    std::string popReadyMsg();
    void pushReadyMsg(const std::string& msg);

    bool closeOnDone() const;
    void setCloseOnDone(bool v);

    bool inFlight() const;
    void setInFlight(bool v);

private:
    int _sockFd;
    int _acceptFd;

    std::string _rx;
    std::string _tx;

    std::time_t _lastSeen;
    std::time_t _phaseSince;

    IoPhase _phase;

    size_t _hdrEnd;
    bool   _hdrReady;

    bool   _isChunked;
    bool   _hasLen;
    size_t _len;

    std::deque<std::string> _readyMsgs;

    bool _closeOnDone;
    bool _inFlight;
};

#endif
