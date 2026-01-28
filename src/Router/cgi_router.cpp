/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi_router.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 20:45:33 by marvin            #+#    #+#             */
/*   Updated: 2026/01/26 00:00:00 by chatgpt           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <sstream>
#include <vector>
#include <map>
#include <string>

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
    long maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd < 0)
        maxfd = 1024;
    for (int fd = start_fd; fd < maxfd; ++fd)
        close(fd);
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
        response.set_body("500 Internal Server Error: CGI extension not found.");
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
        response.set_body("500 Internal Server Error: CGI handler not configured.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    std::string interpreter = it->second;

    // argv: [interpreter, fullpath, NULL]
    std::vector<char*> args;
    args.push_back(const_cast<char*>(interpreter.c_str()));
    args.push_back(const_cast<char*>(fullpath.c_str()));
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
        response.set_body("500 Internal Server Error: Pipe creation failed.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    set_cloexec(pipe_fd_to_cgi[0]);
    set_cloexec(pipe_fd_to_cgi[1]);
    set_cloexec(pipe_fd_from_cgi[0]);
    set_cloexec(pipe_fd_from_cgi[1]);

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipe_fd_to_cgi[0]); close(pipe_fd_to_cgi[1]);
        close(pipe_fd_from_cgi[0]); close(pipe_fd_from_cgi[1]);

        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Fork failed.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    else if (pid == 0)
    {
        if (dup2(pipe_fd_to_cgi[0], STDIN_FILENO) == -1)
            _exit(1);
        if (dup2(pipe_fd_from_cgi[1], STDOUT_FILENO) == -1)
            _exit(1);

        close(pipe_fd_to_cgi[0]); close(pipe_fd_to_cgi[1]);
        close(pipe_fd_from_cgi[0]); close(pipe_fd_from_cgi[1]);
        close_fds_from(3);

        execve(interpreter.c_str(), &args[0], &envp[0]);
        _exit(1);
    }

    // Parent
    close(pipe_fd_to_cgi[0]);
    close(pipe_fd_from_cgi[1]);

    if (!set_nonblocking(pipe_fd_to_cgi[1]) || !set_nonblocking(pipe_fd_from_cgi[0]))
    {
        close(pipe_fd_to_cgi[1]);
        close(pipe_fd_from_cgi[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);

        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Failed to set non-blocking mode for CGI pipes.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }

    const char* body_data = request.body.empty() ? NULL : &request.body[0];
    size_t body_total = request.body.size();
    size_t body_off = 0;

    bool stdin_closed = false;
    bool stdout_closed = false;

    std::string cgi_output;
    const int CGI_TIMEOUT_MS = 30000;
    int elapsed_ms = 0;
    const int SLICE_MS = 100;

    int status = 0;
    bool child_exited = false;

    if (body_total == 0)
    {
        close(pipe_fd_to_cgi[1]);
        stdin_closed = true;
    }

    while (!(stdin_closed && stdout_closed && child_exited))
    {
        struct pollfd fds[2];
        nfds_t nfds = 0;

        if (!stdin_closed)
        {
            fds[nfds].fd = pipe_fd_to_cgi[1];
            fds[nfds].events = POLLOUT;
            fds[nfds].revents = 0;
            nfds++;
        }

        if (!stdout_closed)
        {
            fds[nfds].fd = pipe_fd_from_cgi[0];
            fds[nfds].events = POLLIN | POLLHUP;
            fds[nfds].revents = 0;
            nfds++;
        }

        int poll_ret = 0;
        if (nfds > 0)
            poll_ret = poll(fds, nfds, SLICE_MS);

        if (poll_ret < 0)
            break;

        elapsed_ms += SLICE_MS;
        if (elapsed_ms >= CGI_TIMEOUT_MS)
        {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);

            if (!stdin_closed) close(pipe_fd_to_cgi[1]);
            if (!stdout_closed) close(pipe_fd_from_cgi[0]);

            response.status_code = 504;
            response.reason_phrase = "Gateway Timeout";
            response.set_body("504 Gateway Timeout: CGI script execution timed out.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }

        if (!child_exited)
        {
            pid_t w = waitpid(pid, &status, WNOHANG);
            if (w == pid)
                child_exited = true;
        }

        for (nfds_t i = 0; i < nfds; ++i)
        {
            int fd = fds[i].fd;
            short re = fds[i].revents;

            if (!stdin_closed && fd == pipe_fd_to_cgi[1])
            {
                if (re & (POLLERR | POLLHUP | POLLNVAL))
                {
                    close(pipe_fd_to_cgi[1]);
                    stdin_closed = true;
                }
                else if (re & POLLOUT)
                {
                    if (body_off < body_total)
                    {
                        ssize_t n = write(pipe_fd_to_cgi[1], body_data + body_off, body_total - body_off);
                        if (n <= 0)
                        {
                            close(pipe_fd_to_cgi[1]);
                            stdin_closed = true;
                        }
                        else
                        {
                            body_off += static_cast<size_t>(n);
                            if (body_off >= body_total)
                            {
                                close(pipe_fd_to_cgi[1]);
                                stdin_closed = true;
                            }
                        }
                    }
                    else
                    {
                        close(pipe_fd_to_cgi[1]);
                        stdin_closed = true;
                    }
                }
            }

            if (!stdout_closed && fd == pipe_fd_from_cgi[0])
            {
                if (re & (POLLERR | POLLNVAL))
                {
                    close(pipe_fd_from_cgi[0]);
                    stdout_closed = true;
                }
                else if (re & (POLLIN | POLLHUP))
                {
                    char buffer[8192];
                    ssize_t bytes_read = read(pipe_fd_from_cgi[0], buffer, sizeof(buffer));
                    if (bytes_read > 0)
                        cgi_output.append(buffer, bytes_read);
                    else
                    {
                        close(pipe_fd_from_cgi[0]);
                        stdout_closed = true;
                    }
                }
            }
        }
    }

    if (!child_exited)
        waitpid(pid, &status, 0);

    if (!stdin_closed)
        close(pipe_fd_to_cgi[1]);
    if (!stdout_closed)
        close(pipe_fd_from_cgi[0]);
    if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) && cgi_output.empty())
    {
        HTTPResponse r;
        r.status_code = 502;
        r.reason_phrase = "Bad Gateway";
        r.set_body("502 Bad Gateway: CGI execution failed.");
        r.headers["Content-Type"] = "text/plain";
        r.headers["Content-Length"] = to_string(r.body.size());
        return r;
    }

    return parse_cgi_response(cgi_output);
}