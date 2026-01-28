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
            {
                // skip
            }
            else if (cur == "..")
            {
                if (parts.empty())
                    return false; // above root => reject
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
            // boundary: exact match OR next char is '/'
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

    if (_config.servers.empty())
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: No server configurations available.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    const ServerConfig &server_config = find_server_config(request);

    // 1) Decode %xx (so %2e%2e becomes .. and gets blocked)
    std::string decodedUri;
    if (!url_decode_path(request.uri, decodedUri))
    {
        response.status_code = 400;
        response.reason_phrase = "Bad Request";
        response.set_body("400 Bad Request");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    std::string normUri;
    if (!normalize_uri_path(decodedUri, normUri))
    {
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    const LocationConfig* location_config = find_location_config(normUri, server_config);

    if (!location_config)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    if (!is_method_allowed(*location_config, request.method))
    {
        response.status_code = 405;
        response.reason_phrase = "Method Not Allowed";

        std::string allowed_methods;
        if (location_config->allowMethods.empty())
            allowed_methods = "GET, POST, DELETE";
        else
        {
            for (size_t i = 0; i < location_config->allowMethods.size(); ++i)
            {
                allowed_methods += location_config->allowMethods[i];
                if (i + 1 < location_config->allowMethods.size())
                    allowed_methods += ", ";
            }
        }
        response.headers["Allow"] = allowed_methods;

        response.set_body("405 Method Not Allowed");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    if (location_config->returnCode != 0)
    {
        response.status_code = location_config->returnCode;
        response.reason_phrase = "Redirect";
        response.headers["Location"] = location_config->returnPath;
        response.set_body("Redirecting to " + location_config->returnPath);
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    std::string fullpath = final_path(server_config, *location_config, normUri);

    if (is_cgi_request(*location_config, fullpath))
    {
        response = handle_cgi_request(request, fullpath, *location_config);
        return apply_error_page(server_config, response.status_code, response);
    }
    if (request.method == HTTP_POST)
    {
        response = handle_post_request(request, *location_config, server_config, fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }

    if (request.method == HTTP_DELETE)
    {
        const std::string& lp = location_config->path;
        if (lp != "/" && (normUri.compare(0, lp.size(), lp) != 0 ||
            !(normUri.size() == lp.size() || normUri[lp.size()] == '/')))
        {
            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.set_body("403 Forbidden");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return apply_error_page(server_config, response.status_code, response);
        }

        response = handle_delete_request(fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }
    struct stat sb;
    if (stat(fullpath.c_str(), &sb) != 0)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    if (S_ISDIR(sb.st_mode))
    {
        if (location_config->autoindex == true)
        {
            response = generate_autoindex_response(fullpath);
            return apply_error_page(server_config, response.status_code, response);
        }

        if (!server_config.index.empty())
        {
            std::string index_path = fullpath;
            if (!index_path.empty() && index_path[index_path.size() - 1] != '/')
                index_path += "/";
            index_path += server_config.index;

            struct stat sb_index;
            if (stat(index_path.c_str(), &sb_index) == 0 && S_ISREG(sb_index.st_mode))
            {
                response = serve_static_file(index_path);
                return apply_error_page(server_config, response.status_code, response);
            }

            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.set_body("403 Forbidden");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return apply_error_page(server_config, response.status_code, response);
        }

        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return apply_error_page(server_config, response.status_code, response);
    }

    if (S_ISREG(sb.st_mode))
    {
        response = serve_static_file(fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }

    response.status_code = 501;
    response.reason_phrase = "Not Implemented";
    response.set_body("501 Not Implemented");
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = to_string(response.body.size());
    return apply_error_page(server_config, response.status_code, response);
}
