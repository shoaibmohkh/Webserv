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
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <ctime>

#include <map>
#include <string>
#include <vector>

/* ------------------------ Multipart helpers ------------------------ */

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
    // expects: multipart/form-data; boundary=----XYZ (maybe quoted)
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

    // remove optional quotes
    if (b.size() >= 2 && b[0] == '"' && b[b.size() - 1] == '"')
        b = b.substr(1, b.size() - 2);

    if (b.empty())
        return false;

    outBoundary = b;
    return true;
}

static bool parseFilenameFromDisposition(const std::string& disp, std::string& outName)
{
    // Content-Disposition: form-data; name="file"; filename="testfile.test"
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

static bool extractMultipartFile(const std::map<std::string, std::string>& headers,
                                 const std::vector<char>& body,
                                 std::string& outFilename,
                                 std::vector<char>& outFileBytes)
{
    std::string ct;
    if (!getHeaderCI(headers, "content-type", ct))
        return false;

    std::string boundary;
    if (!extractBoundary(ct, boundary))
        return false;

    if (body.empty())
        return false;

    // body can contain '\0', keep size
    std::string b(&body[0], body.size());

    std::string delim = "--" + boundary;
    std::string nextDelim = "\r\n" + delim;

    // Find first boundary
    size_t p0 = b.find(delim);
    if (p0 == std::string::npos)
        return false;

    // Move to end of boundary line
    size_t p = b.find("\r\n", p0);
    if (p == std::string::npos)
        return false;
    p += 2; // start of part headers

    // Find end of part headers
    size_t hdrEnd = b.find("\r\n\r\n", p);
    if (hdrEnd == std::string::npos)
        return false;

    std::string partHeaders = b.substr(p, hdrEnd - p);

    // Get Content-Disposition line
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

    // File bytes start after "\r\n\r\n"
    size_t dataStart = hdrEnd + 4;

    // Find next boundary (preceded by \r\n)
    size_t p1 = b.find(nextDelim, dataStart);
    if (p1 == std::string::npos)
        return false;

    size_t dataEnd = p1; // bytes end right before "\r\n--boundary"
    if (dataEnd < dataStart)
        return false;

    outFileBytes.assign(b.begin() + dataStart, b.begin() + dataEnd);
    outFilename = fname;
    return true;
}

/* ------------------------ Router methods ------------------------ */

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

    /* ---- choose file bytes: raw body OR multipart extracted bytes ---- */
    std::vector<char> fileBytes;
    std::string mpFilename;

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
        if (!extractMultipartFile(request.headers, request.body, mpFilename, fileBytes))
        {
            response.status_code = 400;
            response.reason_phrase = "Bad Request";
            response.set_body("400 Bad Request: Invalid multipart/form-data.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
    }
    else
    {
        fileBytes = request.body; // raw upload
    }

    /* ---- choose filename: URL name OR multipart filename OR fallback ---- */
    std::string filename;
    size_t last_slash = fullpath.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < fullpath.size())
        filename = fullpath.substr(last_slash + 1);

    // If POST /uploads (no name in URL), use multipart filename
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

    const char* data = fileBytes.empty() ? NULL : &fileBytes[0];
    size_t total = fileBytes.size();
    size_t off = 0;

    while (off < total)
    {
        ssize_t n = write(fd, data + off, total - off);
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

    if (unlink(fullpath.c_str()) < 0)
    {
        if (errno == EACCES || errno == EPERM)
        {
            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.set_body("403 Forbidden: Permission denied.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }

        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to delete the file.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    response.status_code = 200;
    response.reason_phrase = "OK";
    response.set_body("200 OK: File deleted successfully.");
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/plain";
    return response;
}
