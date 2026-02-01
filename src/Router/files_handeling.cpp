/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   files_handeling.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:37:38 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:37:38 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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
        response.set_body("404 Not Found");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return response;
    }

    std::vector<char> body;
    char buffer[8192];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        body.insert(body.end(), buffer, buffer + bytes_read);

    close(fd);

    if (bytes_read < 0)
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error");
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = to_string(response.body.size());
        return response;
    }

    response.status_code = 200;
    response.reason_phrase = "OK";
    response.set_body(body);
    response.headers["Content-Type"] = get_mime_type(fullpath);
    response.headers["Content-Length"] = to_string(response.body.size());
    return response;
}