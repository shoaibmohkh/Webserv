
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   method_router.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:17:07 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/01/28 11:17:07 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

#include <sys/stat.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <ctime>

#include <map>
#include <string>
#include <vector>


static bool getHeaderCI(const std::map<std::string, std::string>& h,
                        const std::string& keyLower,
                        std::string& outVal)
{
    for (std::map<std::string, std::string>::const_iterator it = h.begin(); it != h.end(); ++it)
    {
        std::string k = it->first;
        for (size_t i = 0; i < k.size(); ++i)
            if (k[i] >= 'A' && k[i] <= 'Z') k[i] = char(k[i] - 'A' + 'a');

        if (k == keyLower)
        {
            outVal = it->second;
            return true;
        }
    }
    return false;
}

static std::string trimSpaces(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t')) a++;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}

static bool extractBoundary(const std::string& contentType, std::string& outBoundary)
{
    std::string low = contentType;
    for (size_t i = 0; i < low.size(); ++i)
        if (low[i] >= 'A' && low[i] <= 'Z') low[i] = char(low[i] - 'A' + 'a');

    if (low.find("multipart/form-data") == std::string::npos)
        return false;

    size_t bpos = low.find("boundary=");
    if (bpos == std::string::npos)
        return false;

    std::string b = contentType.substr(bpos + 9);
    b = trimSpaces(b);
    if (b.empty())
        return false;

    if (b.size() >= 2 && b[0] == '"' && b[b.size() - 1] == '"')
        b = b.substr(1, b.size() - 2);

    if (b.empty())
        return false;

    outBoundary = b;
    return true;
}

static bool parseFilenameFromDisposition(const std::string& disp, std::string& outName)
{
    size_t p = disp.find("filename=");
    if (p == std::string::npos)
        return false;

    p += 9;
    if (p >= disp.size())
        return false;

    if (disp[p] == '"')
    {
        size_t q = disp.find('"', p + 1);
        if (q == std::string::npos)
            return false;
        outName = disp.substr(p + 1, q - (p + 1));
        return !outName.empty();
    }

    size_t end = disp.find(';', p);
    if (end == std::string::npos)
        end = disp.size();
    outName = trimSpaces(disp.substr(p, end - p));
    return !outName.empty();
}

static size_t find_mem(const char* hay, size_t hayLen,
                       const char* needle, size_t needleLen,
                       size_t start)
{
    if (!hay || !needle || needleLen == 0 || hayLen < needleLen || start > hayLen - needleLen)
        return (size_t)-1;

    for (size_t i = start; i + needleLen <= hayLen; ++i)
    {
        if (std::memcmp(hay + i, needle, needleLen) == 0)
            return i;
    }
    return (size_t)-1;
}

static bool extractMultipartFileSpan(const std::map<std::string, std::string>& headers,
                                     const std::vector<char>& body,
                                     std::string& outFilename,
                                     size_t& outStart,
                                     size_t& outEnd)
{
    std::string ct;
    if (!getHeaderCI(headers, "content-type", ct))
        return false;

    std::string boundary;
    if (!extractBoundary(ct, boundary))
        return false;

    if (body.empty())
        return false;

    const char* b = &body[0];
    size_t blen = body.size();

    std::string delim = "--" + boundary;
    std::string nextDelim = "\r\n" + delim;

    size_t p0 = find_mem(b, blen, delim.c_str(), delim.size(), 0);
    if (p0 == (size_t)-1)
        return false;

    size_t p = find_mem(b, blen, "\r\n", 2, p0);
    if (p == (size_t)-1)
        return false;
    p += 2;

    size_t hdrEnd = find_mem(b, blen, "\r\n\r\n", 4, p);
    if (hdrEnd == (size_t)-1)
        return false;

    std::string partHeaders(b + p, hdrEnd - p);

    std::string cdLine;
    {
        std::string low = partHeaders;
        for (size_t i = 0; i < low.size(); ++i)
            if (low[i] >= 'A' && low[i] <= 'Z') low[i] = char(low[i] - 'A' + 'a');

        size_t cd = low.find("content-disposition:");
        if (cd != std::string::npos)
        {
            size_t eol = partHeaders.find("\r\n", cd);
            if (eol == std::string::npos)
                eol = partHeaders.size();
            cdLine = partHeaders.substr(cd, eol - cd);
        }
    }

    std::string fname;
    if (!cdLine.empty())
        parseFilenameFromDisposition(cdLine, fname);

    size_t dataStart = hdrEnd + 4;
    size_t p1 = find_mem(b, blen, nextDelim.c_str(), nextDelim.size(), dataStart);
    if (p1 == (size_t)-1)
        return false;

    size_t dataEnd = p1;
    if (dataEnd < dataStart)
        return false;

    outFilename = fname;
    outStart = dataStart;
    outEnd = dataEnd;
    return true;
}


bool Router::is_method_allowed(const LocationConfig& location_config, HTTPMethod method) const
{
    if (location_config.allowMethods.empty())
        return true;

    std::string method_str;
    switch (method)
    {
        case HTTP_GET:    method_str = "GET";    break;
        case HTTP_POST:   method_str = "POST";   break;
        case HTTP_DELETE: method_str = "DELETE"; break;
        default:          return false;
    }

    for (std::vector<std::string>::const_iterator it = location_config.allowMethods.begin();
         it != location_config.allowMethods.end(); ++it)
    {
        if (*it == method_str)
            return true;
    }
    return false;
}

std::string Router::method_to_string(HTTPMethod method) const
{
    switch (method)
    {
        case HTTP_GET:    return "GET";
        case HTTP_POST:   return "POST";
        case HTTP_DELETE: return "DELETE";
        default:          return "";
    }
}

HTTPResponse Router::handle_post_request(const HTTPRequest& request,
                                        const LocationConfig& location_config,
                                        const ServerConfig& server_config,
                                        const std::string& fullpath) const
{
    HTTPResponse response;

    if (!request.body.empty() &&
        server_config.client_Max_Body_Size > 0 &&
        request.body.size() > static_cast<size_t>(server_config.client_Max_Body_Size))
    {
        response.status_code = 413;
        response.reason_phrase = "Payload Too Large";
        response.set_body("413 Payload Too Large");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    if (location_config.uploadEnable == false)
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Uploads are not enabled for this location.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    std::string upload_path;
    if (!location_config.uploadStore.empty() && location_config.uploadStore[0] == '/')
        upload_path = location_config.uploadStore;
    else
        upload_path = server_config.root + "/" + location_config.uploadStore;

    if (!upload_path.empty() && upload_path[upload_path.size() - 1] != '/')
        upload_path += '/';

    struct stat st;
    if (stat(upload_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: upload_store path does not exist or is not a directory.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    std::string mpFilename;
    size_t mpStart = 0;
    size_t mpEnd = 0;

    bool isMultipart = false;
    {
        std::string ct;
        if (getHeaderCI(request.headers, "content-type", ct))
        {
            std::string boundary;
            if (extractBoundary(ct, boundary))
                isMultipart = true;
        }
    }

    if (isMultipart)
    {
        if (!extractMultipartFileSpan(request.headers, request.body, mpFilename, mpStart, mpEnd))
        {
            response.status_code = 400;
            response.reason_phrase = "Bad Request";
            response.set_body("400 Bad Request: Invalid multipart/form-data.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
    }

    std::string filename;
    size_t last_slash = fullpath.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < fullpath.size())
        filename = fullpath.substr(last_slash + 1);

    if (filename.empty() && !mpFilename.empty())
        filename = mpFilename;

    if (filename.empty())
        filename = "upload_" + to_string(static_cast<size_t>(time(NULL))) + ".bin";

    std::string final_upload_path = upload_path + filename;

    int fd = open(final_upload_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to open upload file.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    const char* data = NULL;
    size_t total = 0;

    if (isMultipart)
    {
        if (request.body.empty() || mpEnd < mpStart || mpEnd > request.body.size())
        {
            close(fd);
            response.status_code = 400;
            response.reason_phrase = "Bad Request";
            response.set_body("400 Bad Request: Invalid multipart boundaries.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
        data = &request.body[0] + mpStart;
        total = mpEnd - mpStart;
    }
    else
    {
        if (!request.body.empty())
            data = &request.body[0];
        total = request.body.size();
    }

    size_t off = 0;

    while (off < total)
    {
        size_t chunk = total - off;
        if (chunk > 64 * 1024)
            chunk = 64 * 1024;

        ssize_t n = write(fd, data + off, chunk);
        if (n < 0)
        {
            close(fd);
            response.status_code = 500;
            response.reason_phrase = "Internal Server Error";
            response.set_body("500 Internal Server Error: Unable to write upload file.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
        off += static_cast<size_t>(n);
    }

    close(fd);

    response.status_code = 201;
    response.reason_phrase = "Created";
    response.set_body("201 Created: File uploaded successfully.");
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/plain";
    return response;
}

HTTPResponse Router::handle_delete_request(const std::string& fullpath) const
{
    struct stat sb;
    HTTPResponse response;
    if (stat(fullpath.c_str(), &sb) != 0)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found: File does not exist.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    if (S_ISDIR(sb.st_mode))
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Cannot delete a directory.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    int test_fd = open(fullpath.c_str(), O_WRONLY);
    if (test_fd < 0)
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Permission denied.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    close(test_fd);
    std::string parent_dir = fullpath;
    size_t last_slash = parent_dir.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        parent_dir = parent_dir.substr(0, last_slash);
        if (parent_dir.empty())
            parent_dir = "/";
    }
    else
    {
        parent_dir = ".";
    }
    
    if (access(parent_dir.c_str(), W_OK) != 0)
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Permission denied.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    int fd = open(fullpath.c_str(), O_WRONLY | O_TRUNC);
    if (fd < 0)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to delete the file.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    close(fd);
    
    response.status_code = 200;
    response.reason_phrase = "OK";
    response.set_body("200 OK: File deleted successfully.");
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/plain";
    return response;
}