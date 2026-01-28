#ifndef HTTP10PARSER_HPP
#define HTTP10PARSER_HPP

#include "../HttpRequest.hpp"
#include <string>

namespace http10
{

    bool parseRequest(const std::string& raw,
                      int listenPort,
                      HTTPRequest& outReq,
                      int& outErrCode);
}

#endif
