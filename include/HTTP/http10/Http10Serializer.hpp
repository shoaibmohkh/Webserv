#ifndef HTTP10SERIALIZER_HPP
#define HTTP10SERIALIZER_HPP

#include "../HttpResponse.hpp"
#include <string>

namespace http10
{
    std::string makeError(int code, const char* msg);
    std::string serializeClose(const HTTPResponse& res);
}

#endif
