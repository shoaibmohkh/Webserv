/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi_router.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 20:45:33 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 20:45:33 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

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
