#ifndef NETUTIL_HPP
#define NETUTIL_HPP

#include <vector>
#include <string>

int  makeNonBlocking(int fd);
void closeFd(int fd);
bool parsePortsList(const char* s, std::vector<int>& outPorts);

#endif
