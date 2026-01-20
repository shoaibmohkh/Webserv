#include "../../include/RouterByteHandler.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <cctype>
#include <cstring>      // for memset
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int get_listen_port(int acceptFd)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    std::memset(&addr, 0, sizeof(addr));
    if (getsockname(acceptFd, (sockaddr*)&addr, &len) == 0)
        return (int)ntohs(addr.sin_port);
    return 80;
}

static std::string trim(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace((unsigned char)s[a])) a++;
    size_t b = s.size();
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

static HTTPMethod method_from_str(const std::string& m)
{
    if (m == "GET") return HTTP_GET;
    if (m == "POST") return HTTP_POST;
    if (m == "DELETE") return HTTP_DELETE;
    return HTTP_UNKNOWN;
}

static void parse_host_header(const std::string& hostValue, std::string& outHost, int& outPort)
{
    std::string v = trim(hostValue);
    size_t colon = v.find(':');
    if (colon == std::string::npos)
    {
        outHost = v;
        return;
    }

    outHost = v.substr(0, colon);

    std::string p = v.substr(colon + 1);
    int port = 0;
    for (size_t i = 0; i < p.size(); ++i)
    {
        if (p[i] < '0' || p[i] > '9')
            return;
        port = port * 10 + (p[i] - '0');
        if (port > 65535)
            return;
    }
    if (port > 0)
        outPort = port;
}

static bool parse_raw_request(const std::string& raw, int listenPort, HTTPRequest& req)
{
    size_t he = raw.find("\r\n\r\n");
    if (he == std::string::npos)
        return false;

    std::string headersPart = raw.substr(0, he);
    std::string bodyPart = raw.substr(he + 4);

    // request line
    size_t lineEnd = headersPart.find("\r\n");
    std::string firstLine = (lineEnd == std::string::npos) ? headersPart : headersPart.substr(0, lineEnd);

    std::istringstream iss(firstLine);
    std::string method, uri, version;
    iss >> method >> uri >> version;
    if (method.empty() || uri.empty() || version.empty())
        return false;

    req.method = method_from_str(method);
    req.uri = uri;
    req.version = version;

    // headers
    req.headers.clear();
    if (lineEnd != std::string::npos)
    {
        size_t pos = lineEnd + 2;
        while (pos < headersPart.size())
        {
            size_t eol = headersPart.find("\r\n", pos);
            std::string line = (eol == std::string::npos)
                ? headersPart.substr(pos)
                : headersPart.substr(pos, eol - pos);

            pos = (eol == std::string::npos) ? headersPart.size() : eol + 2;

            if (line.empty())
                continue;

            size_t c = line.find(':');
            if (c == std::string::npos)
                continue;

            std::string key = trim(line.substr(0, c));
            std::string val = trim(line.substr(c + 1));
            if (!key.empty())
                req.headers[key] = val;
        }
    }

    // Router expects these
    req.host = "";
    req.port = listenPort;

    std::map<std::string, std::string>::iterator hit = req.headers.find("Host");
    if (hit != req.headers.end())
        parse_host_header(hit->second, req.host, req.port);

    req.body.assign(bodyPart.begin(), bodyPart.end());
    req.keepAlive = false;
    return true;
}

static std::string reason_default(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "OK";
    }
}

static std::string to_dec(size_t v)
{
    std::ostringstream os;
    os << (unsigned long)v;
    return os.str();
}

static std::string serialize_response(const HTTPResponse& res)
{
    std::map<std::string, std::string> headers = res.headers;

    // enforce required headers
    headers["Content-Length"] = to_dec(res.body.size());
    if (headers.find("Connection") == headers.end())
        headers["Connection"] = "close";
    if (headers.find("Content-Type") == headers.end())
        headers["Content-Type"] = "text/plain";

    int code = res.status_code;
    std::string reason = res.reason_phrase.empty() ? reason_default(code) : res.reason_phrase;

    std::ostringstream oss;
    oss << "HTTP/1.1 " << code << " " << reason << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        oss << it->first << ": " << it->second << "\r\n";
    oss << "\r\n";

    std::string out = oss.str();
    if (!res.body.empty())
        out.append(&res.body[0], res.body.size());
    return out;
}

RouterByteHandler::RouterByteHandler(const std::string& configPath)
: _cfg()
, _router(NULL)
{
    std::ifstream file(configPath.c_str());
    if (!file)
        throw std::runtime_error("Cannot open config file: " + configPath);

    std::stringstream buffer;
    buffer << file.rdbuf();

    Tokenizer tokenizer(buffer.str());
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(tokens);
    _cfg = parser.parse();

    _router = new Router(_cfg);
}

RouterByteHandler::~RouterByteHandler()
{
    delete _router;
    _router = NULL;
}

ByteReply RouterByteHandler::handleBytes(int acceptFd, int /*sockFd*/, const std::string& rawMessage)
{
    HTTPRequest req;
    int listenPort = get_listen_port(acceptFd);

    if (!parse_raw_request(rawMessage, listenPort, req))
    {
        const char* body = "400 Bad Request\n";
        std::string resp;
        resp += "HTTP/1.1 400 Bad Request\r\n";
        resp += "Content-Type: text/plain\r\n";
        resp += "Content-Length: 16\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        resp += body;
        return ByteReply(resp, true);
    }

    HTTPResponse res = _router->handle_route_Request(req);
    std::string rawResp = serialize_response(res);

    return ByteReply(rawResp, true);
}
