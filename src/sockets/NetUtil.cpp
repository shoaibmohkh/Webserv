#include "../../include/sockets/NetUtil.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cctype>

int makeNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void closeFd(int fd)
{
    if (fd >= 0)
        close(fd);
}

static bool parsePortInt(const char* s, int& out)
{
    if (!s || !*s) return false;
    long v = 0;
    for (const char* p = s; *p; ++p)
    {
        if (!std::isdigit(static_cast<unsigned char>(*p)))
            return false;
        v = v * 10 + (*p - '0');
        if (v > 65535)
            return false;
    }
    out = static_cast<int>(v);
    return (out > 0);
}

bool parsePortsList(const char* s, std::vector<int>& outPorts)
{
    outPorts.clear();
    if (!s) return false;

    std::string str(s);
    size_t i = 0;
    while (i < str.size())
    {
        size_t j = str.find(',', i);
        if (j == std::string::npos) j = str.size();
        std::string token = str.substr(i, j - i);

        // trim spaces
        size_t a = 0;
        while (a < token.size() && std::isspace(static_cast<unsigned char>(token[a]))) a++;
        size_t b = token.size();
        while (b > a && std::isspace(static_cast<unsigned char>(token[b-1]))) b--;
        token = token.substr(a, b - a);

        int port = 0;
        if (!parsePortInt(token.c_str(), port))
            return false;
        outPorts.push_back(port);

        i = j + 1;
    }
    return !outPorts.empty();
}
