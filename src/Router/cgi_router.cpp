/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi_router.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:37:16 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:37:16 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cerrno>

#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <dirent.h>

static bool set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return false;
    return true;
}

static void set_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0)
        return;
    fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

static void close_fds_from(int start_fd)
{
    DIR* d = opendir("/proc/self/fd");
    if (!d)
        return;

    int dfd = dirfd(d);

    for (;;)
    {
        errno = 0;
        struct dirent* ent = readdir(d);
        if (!ent)
            break;

        const char* name = ent->d_name;
        if (name[0] < '0' || name[0] > '9')
            continue;

        int fd = std::atoi(name);
        if (fd < start_fd)
            continue;

        if (fd == dfd)
            continue;

        close(fd);
    }

    closedir(d);
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

std::vector<std::string> Router::build_cgi_environment(const HTTPRequest& request,
                                                      const std::string& fullpath) const
{
    std::vector<std::string> env;
    env.push_back("REQUEST_METHOD=" + method_to_string(request.method));

    std::string uri = request.uri;
    size_t qpos = uri.find('?');

    std::string script_name_only;
    std::string query_string;

    if (qpos != std::string::npos)
    {
        script_name_only = uri.substr(0, qpos);
        query_string     = uri.substr(qpos + 1);
    }
    else
    {
        script_name_only = uri;
        query_string     = "";
    }

    env.push_back("QUERY_STRING=" + query_string);

    if (!request.body.empty())
        env.push_back("CONTENT_LENGTH=" + to_string(request.body.size()));
    else
        env.push_back("CONTENT_LENGTH=");

    env.push_back("SCRIPT_FILENAME=" + fullpath);
    env.push_back("SCRIPT_NAME=" + script_name_only);

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

    response.set_body(body_section);
    response.headers["Content-Length"] = to_string(response.body.size());
    return response;
}

bool Router::spawn_cgi(const HTTPRequest& request,
                       const std::string& fullpath,
                       const LocationConfig& location_config,
                       CgiSpawn& outSpawn) const
{
    outSpawn = CgiSpawn();

    std::string::size_type dot_pos = fullpath.find_last_of('.');
    if (dot_pos == std::string::npos)
        return false;

    std::string extension = fullpath.substr(dot_pos);
    std::map<std::string, std::string>::const_iterator it = location_config.cgiExtensions.find(extension);
    if (it == location_config.cgiExtensions.end())
        return false;

    std::string interpreter = it->second;

    std::vector<char*> args;
    args.push_back(const_cast<char*>(interpreter.c_str()));
    args.push_back(const_cast<char*>(fullpath.c_str()));
    args.push_back(NULL);

    std::vector<std::string> env_vars = build_cgi_environment(request, fullpath);
    std::vector<char*> envp;
    for (size_t i = 0; i < env_vars.size(); ++i)
        envp.push_back(const_cast<char*>(env_vars[i].c_str()));
    envp.push_back(NULL);

    int pipe_to_cgi[2];
    int pipe_from_cgi[2];

    if (pipe(pipe_to_cgi) == -1 || pipe(pipe_from_cgi) == -1)
        return false;

    set_cloexec(pipe_to_cgi[0]);
    set_cloexec(pipe_to_cgi[1]);
    set_cloexec(pipe_from_cgi[0]);
    set_cloexec(pipe_from_cgi[1]);

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipe_to_cgi[0]); close(pipe_to_cgi[1]);
        close(pipe_from_cgi[0]); close(pipe_from_cgi[1]);
        return false;
    }
    else if (pid == 0)
    {
        if (dup2(pipe_to_cgi[0], STDIN_FILENO) == -1)
            _exit(1);
        if (dup2(pipe_from_cgi[1], STDOUT_FILENO) == -1)
            _exit(1);

        close(pipe_to_cgi[0]); close(pipe_to_cgi[1]);
        close(pipe_from_cgi[0]); close(pipe_from_cgi[1]);

        close_fds_from(3);

        execve(interpreter.c_str(), &args[0], &envp[0]);
        _exit(1);
    }
    close(pipe_to_cgi[0]);
    close(pipe_from_cgi[1]);

    if (!set_nonblocking(pipe_to_cgi[1]) || !set_nonblocking(pipe_from_cgi[0]))
    {
        close(pipe_to_cgi[1]);
        close(pipe_from_cgi[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return false;
    }

    outSpawn.pid  = pid;
    outSpawn.fdIn = pipe_to_cgi[1];
    outSpawn.fdOut= pipe_from_cgi[0];
    return true;
}
