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

Router::Router(const Config& config) : _config(config)
{
    std::cout << "Router initialized with " << _config.servers.size() << " server(s)." << std::endl;
}

Router::~Router()
{
    std::cout << "Router destroyed." << std::endl;
}

HTTPResponse Router::handle_route_Request(const HTTPRequest& request) const
{
    HTTPResponse response;
    if (_config.servers.empty())
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: No server configurations available.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(_config.servers[0], response.status_code, response);
    }

    const ServerConfig &server_config = find_server_config(request);
    const LocationConfig* location_config = find_location_config(request.uri, server_config);
    if (!location_config)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
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

        response.body = "405 Method Not Allowed";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(server_config, response.status_code, response);
    }
    if (location_config->returnCode != 0)
    {
        response.status_code = location_config->returnCode;
        response.reason_phrase = "Redirect";
        response.headers["Location"] = location_config->returnPath;
        response.body = "Redirecting to " + location_config->returnPath;
        response.headers["Content-Length"] = to_string(response.body.size());

        return apply_error_page(server_config, response.status_code, response);
    }
    std::string fullpath = final_path(server_config, *location_config, request.uri);

    struct stat sb;
    if (request.method == HTTP_POST)
    {
        response = handle_post_request(request, *location_config, server_config, fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }
    if (stat(fullpath.c_str(), &sb) != 0)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
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
            if (index_path[index_path.size() - 1] != '/')
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
            response.body = "403 Forbidden";
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";

            return apply_error_page(server_config, response.status_code, response);
        }
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.body = "403 Forbidden";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(server_config, response.status_code, response);
    }
    if (S_ISREG(sb.st_mode))
    {
        if (is_cgi_request(*location_config, fullpath))
        {
            response = handle_cgi_request(request, fullpath, *location_config);
            return apply_error_page(server_config, response.status_code, response);
        }
        else
        {
            response = serve_static_file(fullpath);
            return apply_error_page(server_config, response.status_code, response);
        }
    }
    if (request.method == HTTP_POST)
    {
        response = handle_post_request(request, *location_config, server_config, fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }
    if (request.method == HTTP_DELETE)
    {
        response = handle_delete_request(fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }
    response.status_code = 501;
    response.reason_phrase = "Not Implemented";
    response.body = "Router logic not completed yet.\n";
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = to_string(response.body.size());

    return apply_error_page(server_config, response.status_code, response);
}

const LocationConfig* Router::find_location_config(const std::string &uri, const ServerConfig& server_config) const
{
    const LocationConfig *best_match = NULL;
    int best_length = 0;

    if (server_config.locations.empty())
        return NULL;
    const LocationConfig* root_location = NULL;
    for (std::vector<LocationConfig>::const_iterator it = server_config.locations.begin(); it != server_config.locations.end(); ++it)
    {
        const LocationConfig &loc = *it;
        const std::string& loc_path = loc.path;
        if (loc_path == "/")
            root_location = &loc;
        if (uri.size() < loc_path.size())
            continue;
        if (uri.compare(0, loc.path.length(), loc.path) == 0)
        {
            int length = loc.path.length();
            if (length > best_length)
            {
                best_length = length;
                best_match = &loc;
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