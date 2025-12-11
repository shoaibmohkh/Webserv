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

bool Router::is_method_allowed(const LocationConfig& location_config, HTTPMethod method) const
{
    if (location_config.allowMethods.empty())
        return true;
    std::string method_str;
    switch (method)
    {
        case HTTP_GET:
            method_str = "GET";
            break;
        case HTTP_POST:
            method_str = "POST";
            break;
        case HTTP_DELETE:
            method_str = "DELETE";
            break;
        default:
            return false;
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
        case HTTP_GET:
            return "GET";
        case HTTP_POST:
            return "POST";
        case HTTP_DELETE:
            return "DELETE";
        default:
            return "";
    }
}

std::string Router::to_string(int value) const
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string Router::final_path(const ServerConfig& server,
                               const LocationConfig& location,
                               const std::string& uri) const
{
    std::string loc = location.path;
    std::string root = server.root;
    std::string remaining;

    if (loc == "/")
    {
        remaining = uri;
    }
    else if (uri.compare(0, loc.length(), loc) == 0)
    {
        remaining = uri.substr(loc.length());
        if (remaining.empty())
            remaining = "/";
    }
    else
        remaining = uri;
    if (!root.empty() && root[root.length() - 1] != '/')
        root += "/";
    if (!remaining.empty() && remaining[0] == '/')
        remaining = remaining.substr(1);
    std::string full = root + remaining;
    for (size_t pos = full.find("//"); pos != std::string::npos; pos = full.find("//"))
        full.erase(pos, 1);

    return full;
}

HTTPResponse Router::generate_autoindex_response(const std::string& path) const
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        HTTPResponse response;
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    std::string html;
    html += "<html>\n<head><title>Index of " + path + "</title></head>\n<body>\n";
    html += "<h1>Index of " + path + "</h1>\n<hr>\n<ul>\n";
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullEntryPath = path;
        if (fullEntryPath[fullEntryPath.size() - 1] != '/')
            fullEntryPath += "/";
        fullEntryPath += name;

        struct stat st;
        if (stat(fullEntryPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        {
            html += "<li><a href=\"";
            html += name + "/";
            html += "\">";
            html += name + "/";
            html += "</a></li>\n";
        }
        else
        {
            html += "<li><a href=\"";
            html += name;
            html += "\">";
            html += name;
            html += "</a></li>\n";
        }
    }

    closedir(dir);

    html += "</ul>\n<hr>\n</body>\n</html>";

    HTTPResponse response;
    response.status_code = 200;
    response.reason_phrase = "OK";
    response.body = html;
    response.headers["Content-Type"] = "text/html";
    response.headers["Content-Length"] = to_string(response.body.size());

    return response;
}
    

HTTPResponse Router::handle_route_Request(const HTTPRequest& request) const
{
    const ServerConfig &server_config = find_server_config(request);
    const LocationConfig* location_config = find_location_config(request.uri, server_config);
    if (!location_config)
    {
        // "/"
    }
    if (!is_method_allowed(*location_config, request.method))
    {
        HTTPResponse response;
        response.status_code = 405;
        response.reason_phrase = "Method Not Allowed";
        
        std::string allowed_methods;
        if (location_config->allowMethods.empty())
            allowed_methods = "GET, POST, DELETE";
        else
        {
            for (int i = 0; i < location_config->allowMethods.size(); ++i)
            {
                allowed_methods += location_config->allowMethods[i];
                if (i != location_config->allowMethods.size() - 1)
                    allowed_methods += ", ";
            }
        }
        response.headers["Allow"] = allowed_methods;
        response.body = "405 Method Not Allowed";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return response;
    }
    if (location_config->returnCode != 0)
    {
        HTTPResponse response;
        response.status_code = location_config->returnCode;
        response.reason_phrase = "Redirect";
        response.headers["Location"] = location_config->returnPath;
        response.body = "Redirecting to " + location_config->returnPath;
        response.headers["Content-Length"] = to_string(response.body.size());
    
        return response;
    }
    std::string fullpath = final_path(server_config, *location_config, request.uri);
    struct stat sb;
    if (stat(fullpath.c_str(), &sb) != 0)
    {
        HTTPResponse response;
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    if (S_ISDIR(sb.st_mode))
    {
        if (location_config->autoindex == true)
            return generate_autoindex_response(fullpath);
        if (!server_config.index.empty())
        {
            std::string index_path = fullpath;
            if (index_path[index_path.size() - 1] != '/')
                index_path += "/";
            index_path += server_config.index;
            struct stat sb_index;
            if (stat(index_path.c_str(), &sb_index) == 0 && S_ISREG(sb_index.st_mode))
            {
                HTTPResponse response;
                response.status_code = 200;
                response.reason_phrase = "OK";
                response.body = "Serving index file: " + index_path + "\n";
                response.headers["Content-Length"] = to_string(response.body.size());
                response.headers["Content-Type"] = "text/plain";
                return response;
            }
            HTTPResponse response;
            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.body = "403 Forbidden";
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
        HTTPResponse response;
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.body = "403 Forbidden";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    if (S_ISREG(sb.st_mode))
    {
        if (is_cgi_request(*location_config, fullpath))
        {
            return handle_cgi_request(request, fullpath, *location_config);
        }
        else
        {
            HTTPResponse response;
            response.status_code = 200;
            response.reason_phrase = "OK";
            response.body = "Serving file: " + fullpath + "\n";
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
    }
    HTTPResponse temp;
    temp.status_code = 501;
    temp.reason_phrase = "Not Implemented";
    temp.body = "Router logic not completed yet.\n";
    temp.headers["Content-Type"] = "text/plain";
    temp.headers["Content-Length"] = to_string(temp.body.size());
    return temp;
}

const LocationConfig* Router::find_location_config(const std::string &uri, const ServerConfig& server_config) const
{
    const LocationConfig *best_match = NULL;
    int best_length = 0;

    if (server_config.locations.empty())
        return nullptr;
    const LocationConfig* root_location = nullptr;
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
    if (best_match == NULL && root_location != nullptr)
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
