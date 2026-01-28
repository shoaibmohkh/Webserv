#include "../../include/sockets/ListenPort.hpp"

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int get_listen_port(int acceptFd)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    if (getsockname(acceptFd, (sockaddr*)&addr, &len) == 0)
        return (int)ntohs(addr.sin_port);
    return 80;
}
