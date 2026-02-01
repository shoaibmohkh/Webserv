/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   NetChannel.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:42:07 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:42:07 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef NETCHANNEL_HPP
#define NETCHANNEL_HPP

#include <string>
#include <deque>
#include <ctime>
#include <sys/types.h>

enum IoPhase
{
    PHASE_RECV_HEADERS = 0,
    PHASE_RECV_BODY    = 1,
    PHASE_SEND         = 2,
    PHASE_SHUTDOWN     = 3
};

struct CgiSession
{
    bool   active;
    pid_t  pid;
    int    fdIn;   // write body -> CGI stdin
    int    fdOut;  // read CGI stdout

    std::string inBody;
    size_t      inOff;

    std::string outBuf;

    std::time_t startTs;
    int         timeoutSec;

    CgiSession()
    : active(false), pid(-1), fdIn(-1), fdOut(-1),
      inBody(), inOff(0), outBuf(),
      startTs(0), timeoutSec(30)
    {}
};

class NetChannel
{
public:
    struct UploadSession
    {
        bool        active;
        int         fd;
        std::string raw;        // full HTTP request (store it to avoid copies)
        size_t      dataStart;  // file bytes start offset in raw
        size_t      dataEnd;    // file bytes end offset in raw
        size_t      off;        // bytes already written

        UploadSession()
        : active(false), fd(-1), raw(), dataStart(0), dataEnd(0), off(0)
        {}
    };

    struct FileSendSession
    {
        bool        active;
        int         fd;
        std::string path;
        size_t      off;

        FileSendSession()
        : active(false), fd(-1), path(), off(0)
        {}
    };

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
    void        pushReadyMsg(std::string& msg);
    void        pushReadyMsg(const std::string& msg);

    bool closeOnDone() const;
    void setCloseOnDone(bool v);

    bool inFlight() const;
    void setInFlight(bool v);

    CgiSession& cgi();

    UploadSession& upload();

    FileSendSession& file();
    const FileSendSession& file() const;

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

    CgiSession _cgi;

    UploadSession _upload;

    FileSendSession _file;
};

#endif
