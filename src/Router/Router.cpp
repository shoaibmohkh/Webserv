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
    std::string root = location.root.empty() ? server.root : location.root;
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
    
std::string Router::get_mime_type(const std::string& filepath) const
{
    std::string::size_type dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos)
        return "application/octet-stream";

    std::string extension = filepath.substr(dot_pos + 1);
    if (extension == "html" || extension == "htm")
        return "text/html";
    if (extension == "css")
        return "text/css";
    if (extension == "js")
        return "application/javascript";
    if (extension == "png")
        return "image/png";
    if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    if (extension == "gif")
        return "image/gif";
    if (extension == "txt")
        return "text/plain";
    if (extension == "pdf")
        return "application/pdf";
    return "application/octet-stream";
}

HTTPResponse Router::serve_static_file(const std::string& fullpath) const
{
    HTTPResponse response;
    int fd = open(fullpath.c_str(), O_RDONLY);
    if (fd < 0)
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    std::string body;
    char buffer[8192];
    ssize_t bytes_read = 0;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        body.append(buffer, bytes_read);
    }
    close(fd);
    if (bytes_read < 0)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    response.status_code = 200;
    response.reason_phrase = "OK";
    response.body = body;
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = get_mime_type(fullpath);
    return response;
}

bool Router::is_cgi_request(const LocationConfig& location_config, const std::string& fullpath) const
{
    std::string::size_type dot_pos = fullpath.find_last_of('.');
    if (dot_pos == std::string::npos)
        return false;

    std::string extension = fullpath.substr(dot_pos);
    std::map<std::string, std::string>::const_iterator it = location_config.cgiExtensions.find(extension);
    return it != location_config.cgiExtensions.end();
}

std::vector<std::string> Router::build_cgi_environment(const HTTPRequest& request, const std::string& fullpath) const
{
    std::vector<std::string> env;
    env.push_back("REQUEST_METHOD=" + method_to_string(request.method));
    std::string query_string;
    std::string uri = request.uri;
    size_t qpos = uri.find('?');
    if (qpos != std::string::npos)
        query_string = uri.substr(qpos + 1);
    else
        query_string = "";
    
    env.push_back("QUERY_STRING=" + query_string);
    if (!request.body.empty())
        env.push_back("CONTENT_LENGTH=" + to_string(request.body.size()));
    else
        env.push_back("CONTENT_LENGTH=");

    env.push_back("SCRIPT_FILENAME=" + fullpath);
    env.push_back("SCRIPT_NAME=" + uri);
    env.push_back("SERVER_NAME=" + request.host);
    env.push_back("SERVER_PORT=" + to_string(request.port));
    env.push_back("SERVER_PROTOCOL=" + request.version);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");

    for (std::map<std::string, std::string>::const_iterator it = request.headers.begin();
         it != request.headers.end(); ++it)
    {
        std::string key = it->first;
        for (std::string::size_type i = 0; i < key.size(); ++i)
        {
            if (key[i] >= 'a' && key[i] <= 'z')
                key[i] -= 32;
            else if (key[i] == '-')
                key[i] = '_';
        }
        env.push_back("HTTP_" + key + "=" + it->second);
    }
    return env;
}

HTTPResponse Router::parse_cgi_response(const std::string& cgi_output) const
{
    HTTPResponse response;
    response.status_code = 200;
    response.reason_phrase = "OK";

    std::string::size_type header_end = cgi_output.find("\r\n\r\n");
    std::string::size_type separator_length = 4;

    if (header_end == std::string::npos)
    {
        header_end = cgi_output.find("\n\n");
        separator_length = 2;
    }

    std::string header_section;
    std::string body_section;

    if (header_end != std::string::npos)
    {
        header_section = cgi_output.substr(0, header_end);
        body_section = cgi_output.substr(header_end + separator_length);
    }
    else
        body_section = cgi_output;

    std::map<std::string, std::string> headers;

    if (!header_section.empty())
    {
        std::istringstream header_stream(header_section);
        std::string line;
        while (std::getline(header_stream, line))
        {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);

            if (line.empty())
                continue;

            std::string::size_type pos = line.find(':');
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);

            headers[key] = value;
        }
    }
    if (headers.find("Status") != headers.end())
    {
        std::stringstream ss(headers["Status"]);
        int status_code;
        ss >> status_code;

        if (!ss.fail())
        {
            response.status_code = status_code;
            std::string reason;
            std::getline(ss, reason);
            if (!reason.empty() && reason[0] == ' ')
                reason.erase(0, 1);
            if (!reason.empty())
                response.reason_phrase = reason;
        }
        headers.erase("Status");
    }
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
        response.headers[it->first] = it->second;
    if (response.headers.find("Content-Type") == response.headers.end())
        response.headers["Content-Type"] = "text/html";
    response.body = body_section;
    response.headers["Content-Length"] = to_string(response.body.size());

    return response;
}


HTTPResponse Router::handle_cgi_request(const HTTPRequest& request,
                                        const std::string& fullpath,
                                        const LocationConfig& location_config) const
{
    HTTPResponse response;
    std::string::size_type dot_pos = fullpath.find_last_of('.');
    if (dot_pos == std::string::npos)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: CGI extension not found.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    std::string extension = fullpath.substr(dot_pos);
    std::map<std::string, std::string>::const_iterator it = location_config.cgiExtensions.find(extension);

    if (it == location_config.cgiExtensions.end())
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: CGI handler not configured.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    std::string interpreter = it->second;

    std::string::size_type pos_slash = fullpath.find_last_of('/');
    std::string script_dir = (pos_slash != std::string::npos) ? fullpath.substr(0, pos_slash) : "";
    std::string script_name = (pos_slash != std::string::npos) ? fullpath.substr(pos_slash + 1) : fullpath;

    std::vector<char*> args;
    args.push_back(const_cast<char*>(interpreter.c_str()));
    args.push_back(const_cast<char*>(script_name.c_str()));
    args.push_back(NULL);
    std::vector<std::string> env_vars = build_cgi_environment(request, fullpath);
    std::vector<char*> envp;
    for (size_t i = 0; i < env_vars.size(); ++i)
        envp.push_back(const_cast<char*>(env_vars[i].c_str()));
    envp.push_back(NULL);

    int pipe_fd_to_cgi[2];
    int pipe_fd_from_cgi[2];

    if (pipe(pipe_fd_to_cgi) == -1 || pipe(pipe_fd_from_cgi) == -1)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: Pipe creation failed.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: Fork failed.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    else if (pid == 0)
    {
        dup2(pipe_fd_to_cgi[0], STDIN_FILENO);
        dup2(pipe_fd_from_cgi[1], STDOUT_FILENO);

        close(pipe_fd_to_cgi[1]);
        close(pipe_fd_from_cgi[0]);
        if (!script_dir.empty())
            chdir(script_dir.c_str());

        execve(interpreter.c_str(), &args[0], &envp[0]);

        exit(1);
    }
    close(pipe_fd_to_cgi[0]);
    close(pipe_fd_from_cgi[1]);

    if (!request.body.empty())
        write(pipe_fd_to_cgi[1], request.body.c_str(), request.body.size());

    close(pipe_fd_to_cgi[1]);

    std::string cgi_output;
    char buffer[8192];
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_fd_from_cgi[0], buffer, sizeof(buffer))) > 0)
        cgi_output.append(buffer, bytes_read);

    close(pipe_fd_from_cgi[0]);

    int status;
    waitpid(pid, &status, 0);

    return parse_cgi_response(cgi_output);
}


HTTPResponse Router::handle_route_Request(const HTTPRequest& request) const
{
    // FIX: Add safety check for empty server configs
    if (_config.servers.empty())
    {
        HTTPResponse response;
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: No server configurations available.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    const ServerConfig &server_config = find_server_config(request);
    const LocationConfig* location_config = find_location_config(request.uri, server_config);
    if (!location_config)
    {
        HTTPResponse response;
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
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
            for (std::vector<std::string>::size_type i = 0; i < location_config->allowMethods.size(); ++i)
            {
                allowed_methods += location_config->allowMethods[i];
                if (i + 1 != location_config->allowMethods.size())
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
                return serve_static_file(index_path);
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
            return serve_static_file(fullpath);
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