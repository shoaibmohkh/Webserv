#ifndef NETCHANNEL_HPP
#define NETCHANNEL_HPP

#include <string>
#include <deque>
#include <ctime>
#include <cstddef>

enum IoPhase {
    PHASE_RECV_HEADERS,
    PHASE_RECV_BODY,
    PHASE_SEND,
    PHASE_SHUTDOWN
};

class NetChannel
{
private:
    int         _sockFd;
    int         _acceptFd;     // which listening socket accepted this connection

    std::string _rx;
    std::string _tx;

    std::time_t _lastSeen;
    std::time_t _phaseSince;

    IoPhase     _phase;

    // Framing info (minimal HTTP message boundary support)
    size_t      _hdrEnd;        // index of \r\n\r\n end (points to first '\r' of that)
    bool        _hdrReady;

    bool        _isChunked;
    bool        _hasLen;
    size_t      _len;

    // When a full request is framed, we enqueue it here (raw bytes ready for Partner 2)
    std::deque<std::string> _readyMsgs;

    // Handler can ask for close-after-write (e.g., HTTP/1.0)
    bool        _closeOnDone;

public:
    NetChannel();
    NetChannel(int sockFd, int acceptFd);

    int         sockFd() const;
    int         acceptFd() const;

    std::string& rxBuffer();
    std::string& txBuffer();

    std::time_t lastSeen() const;
    void        markSeen();

    std::time_t phaseSince() const;
    void        markPhaseSince();

    IoPhase     phase() const;
    void        setPhase(IoPhase p);

    // Framing helpers
    void        resetFraming();
    bool        hdrReady() const;
    size_t      hdrEnd() const;

    void        setHdrEnd(size_t pos);

    bool        isChunked() const;
    void        setChunked(bool v);

    bool        hasLen() const;
    void        setHasLen(bool v);

    size_t      len() const;
    void        setLen(size_t n);

    // Request queue
    bool        hasReadyMsg() const;
    std::string popReadyMsg();
    void        pushReadyMsg(const std::string& msg);

    bool        closeOnDone() const;
    void        setCloseOnDone(bool v);
};

#endif
