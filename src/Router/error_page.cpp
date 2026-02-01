/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error_page.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:37:32 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:37:32 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

// Apply_error_page: Explanation:
// 1. What apply_error_page Is Responsible For
// Simple definition
// This function replaces the response body with a custom error page if the server configuration defines one for that status code.

// Important:
// It does not change the status code
// It does not change the logic
// It only changes the body + headers

// If no custom error page exists → it returns the response unchanged.
// That is exactly what the subject requires.
//-----------------------------------------------------------
// 2. When This Function Is Called
// You call apply_error_page every time before returning a response from the router.

// Example:
// 404 Not Found
// 403 Forbidden
// 405 Method Not Allowed
// 500 Internal Server Error
// Even 301 / 302 (if configured)

// So this function acts like a final decoration step.
//-----------------------------------------------------------
// 3. How It Works
// Request:
// GET /missing.html HTTP/1.1

// Original response before apply_error_page:
// HTTP/1.1 404 Not Found
// Content-Type: text/plain
// Content-Length: 13
// 404 Not Found

// After apply_error_page:
// HTTP/1.1 404 Not Found
// Content-Type: text/html
// Content-Length: 47
// <html>
//   <h1>Custom 404 Page</h1>
// </html>

#include "../../include/Router_headers/Router.hpp"

#include <fcntl.h>
#include <unistd.h>

static std::string reasonFromCode(int code)
{
    if (code == 400) return "Bad Request";
    if (code == 403) return "Forbidden";
    if (code == 404) return "Not Found";
    if (code == 405) return "Method Not Allowed";
    if (code == 413) return "Payload Too Large";
    if (code == 500) return "Internal Server Error";
    if (code == 505) return "HTTP Version Not Supported";
    return "Error";
}

HTTPResponse Router::apply_error_page(const ServerConfig& server_config,
                                     int status_code,
                                     HTTPResponse response) const
{
    if (response.status_code != status_code)
        return response;

    std::map<int, std::string>::const_iterator it =
        server_config.error_Pages.find(status_code);
    if (it == server_config.error_Pages.end())
        return response;

    std::string path = server_config.root + it->second;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return response;

    std::vector<char> body;
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        body.insert(body.end(), buf, buf + n);

    close(fd);

    if (n < 0)
        return response;
    response.status_code = status_code;
    response.reason_phrase = reasonFromCode(status_code);

    response.set_body(body);
    response.headers["Content-Type"] = "text/html";
    response.headers["Content-Length"] = to_string(response.body.size());
    return response;
}
