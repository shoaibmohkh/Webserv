/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 02:31:02 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 02:31:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <sys/stat.h>
#include <cstring>

Router::Router(const Config& config) : _config(config) {}
Router::~Router() {}


static bool url_decode_path(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());

    for (size_t i = 0; i < in.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c == '%')
        {
            if (i + 2 >= in.size())
                return false;

            unsigned char h1 = static_cast<unsigned char>(in[i + 1]);
            unsigned char h2 = static_cast<unsigned char>(in[i + 2]);
            if (!std::isxdigit(h1) || !std::isxdigit(h2))
                return false;

            int v1 = (h1 <= '9') ? (h1 - '0') : (std::tolower(h1) - 'a' + 10);
            int v2 = (h2 <= '9') ? (h2 - '0') : (std::tolower(h2) - 'a' + 10);

            out.push_back(static_cast<char>((v1 << 4) | v2));
            i += 2;
        }
        else
        {
            out.push_back(static_cast<char>(c));
        }
    }
    return true;
}

static bool normalize_uri_path(const std::string& rawUri, std::string& out)
{
    std::string path = rawUri;
    size_t q = path.find('?');
    if (q != std::string::npos)
        path = path.substr(0, q);

    if (path.empty() || path[0] != '/')
        path = "/" + path;

    std::vector<std::string> parts;
    std::string cur;

    for (size_t i = 0; i <= path.size(); ++i)
    {
        char ch = (i == path.size()) ? '/' : path[i];
        if (ch == '/')
        {
            if (cur.empty() || cur == ".")
            {}
            else if (cur == "..")
            {
                if (parts.empty())
                    return false;
                parts.pop_back();
            }
            else
            {
                parts.push_back(cur);
            }
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }

    out = "/";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        out += parts[i];
        if (i + 1 < parts.size())
            out += "/";
    }
    return true;
}

const LocationConfig* Router::find_location_config(const std::string &uri,
                                                  const ServerConfig& server_config) const
{
    const LocationConfig *best_match = NULL;
    int best_length = 0;

    if (server_config.locations.empty())
        return NULL;

    const LocationConfig* root_location = NULL;

    for (std::vector<LocationConfig>::const_iterator it = server_config.locations.begin();
         it != server_config.locations.end(); ++it)
    {
        const LocationConfig &loc = *it;
        const std::string& loc_path = loc.path;

        if (loc_path == "/")
            root_location = &loc;

        if (uri.size() < loc_path.size())
            continue;

        if (uri.compare(0, loc_path.length(), loc_path) == 0)
        {
            if (uri.size() == loc_path.size() || uri[loc_path.size()] == '/')
            {
                int length = (int)loc_path.length();
                if (length > best_length)
                {
                    best_length = length;
                    best_match = &loc;
                }
            }
        }
    }

    if (best_match == NULL && root_location != NULL)
        return root_location;

    return best_match;
}


const ServerConfig &Router::find_server_config(const HTTPRequest& request) const
{
    if (_config.servers.empty())
        throw std::runtime_error("No server configurations available.");

    std::string host = request.host;
    std::transform(host.begin(), host.end(), host.begin(), ::tolower);

    std::vector<const ServerConfig*> candidates;

    for (std::vector<ServerConfig>::const_iterator it = _config.servers.begin();
         it != _config.servers.end(); ++it)
    {
        const ServerConfig &server = *it;
        if (server.port == request.port)
            candidates.push_back(&server);
    }

    if (candidates.empty())
        return _config.servers[0];

    if (candidates.size() == 1)
        return *candidates[0];

    if (host.empty())
        return *candidates[0];

    for (std::vector<const ServerConfig*>::iterator it = candidates.begin();
         it != candidates.end(); ++it)
    {
        const ServerConfig &server = **it;

        std::string server_name = server.server_name;
        std::transform(server_name.begin(), server_name.end(), server_name.begin(), ::tolower);

        if (server_name == host)
            return server;
    }

    return *candidates[0];
}
HTTPResponse Router::handle_route_Request(const HTTPRequest& request) const
{
    HTTPResponse response;

    bool isHead = (request.method == HTTP_HEAD);
    HTTPMethod effectiveMethod = isHead ? HTTP_GET : request.method;

    if (_config.servers.empty())
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return response;
    }

    const ServerConfig& server = find_server_config(request);

    std::string decoded;
    if (!url_decode_path(request.uri, decoded))
    {
        response.status_code = 400;
        response.reason_phrase = "Bad Request";
        response.set_body("400 Bad Request");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 400, response);
    }
    std::string norm;
    if (!normalize_uri_path(decoded, norm))
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 403, response);
    }
    const LocationConfig* loc = find_location_config(norm, server);
    if (!loc)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 404, response);
    }
    if (loc->returnCode != 0)
    {
        response.status_code = loc->returnCode;
        response.reason_phrase = "Moved Permanently";
        response.headers["Location"] = loc->returnPath;
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = "0";
        return response;
    }
    if (!is_method_allowed(*loc, effectiveMethod))
    {
        response.status_code = 405;
        response.reason_phrase = "Method Not Allowed";

        std::string allow;
        for (size_t i = 0; i < loc->allowMethods.size(); ++i)
        {
            allow += loc->allowMethods[i];
            if (i + 1 < loc->allowMethods.size())
                allow += ", ";
        }

        response.headers["Allow"] = allow;
        response.set_body("405 Method Not Allowed");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 405, response);
    }

    std::string fullpath = final_path(server, *loc, norm);

    struct stat st;
    if (stat(fullpath.c_str(), &st) != 0)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 404, response);
    }

    char realRoot[PATH_MAX];
    char realFull[PATH_MAX];

    if (!realpath(server.root.c_str(), realRoot) ||
        !realpath(fullpath.c_str(), realFull) ||
        std::strncmp(realFull, realRoot, std::strlen(realRoot)) != 0)
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 403, response);
    }
    if (access(fullpath.c_str(), R_OK) != 0)
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return apply_error_page(server, 403, response);
    }
    if (!loc->cgiExtensions.empty())
    {
        if (!is_cgi_request(*loc, fullpath))
        {
            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.set_body("403 Forbidden");
            response.headers["Content-Type"] = "text/plain";
            response.headers["Content-Length"] = to_string(response.body.size());
            return apply_error_page(server, 403, response);
        }
    }
    if (effectiveMethod == HTTP_POST)
    {
        response = handle_post_request(request, *loc, server, fullpath);
        return apply_error_page(server, response.status_code, response);
    }

    if (effectiveMethod == HTTP_DELETE)
    {
        response = handle_delete_request(fullpath);
        return apply_error_page(server, response.status_code, response);
    }

    if (S_ISDIR(st.st_mode))
    {
        if (loc->autoindex)
            response = generate_autoindex_response(fullpath);
        else
        {
            std::string index = fullpath + "/" + server.index;
            if (stat(index.c_str(), &st) == 0)
                response = serve_static_file(index);
            else
            {
                response.status_code = 404;
                response.reason_phrase = "Not Found";
                response.set_body("404 Not Found");
                response.headers["Content-Type"] = "text/plain";
                response.headers["Content-Length"] = to_string(response.body.size());
                return apply_error_page(server, 404, response);
            }
        }
    }
    else
    {
        response = serve_static_file(fullpath);
    }

    if (isHead)
    {
        std::string len = response.headers["Content-Length"];
        response.body.clear();
        response.headers["Content-Length"] = len;
    }

    return response;
}
