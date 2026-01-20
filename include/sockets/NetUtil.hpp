#ifndef NETUTIL_HPP
#define NETUTIL_HPP

#include <vector>
#include <string>

int  makeNonBlocking(int fd);
void closeFd(int fd);

// parse "8080,8081,9090" into {8080,8081,9090}
bool parsePortsList(const char* s, std::vector<int>& outPorts);

#endif
