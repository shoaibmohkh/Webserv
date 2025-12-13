/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error_page.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 21:51:35 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 21:51:35 by marvin           ###   ########.fr       */
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



HTTPResponse Router::apply_error_page(const ServerConfig& server_config, int status_code, HTTPResponse response) const
{
    std::map<int, std::string>::const_iterator it = server_config.error_Pages.find(status_code);
    if (it == server_config.error_Pages.end())
        return response;
    std::string error_page_path = server_config.root + it->second;
    int fd = open(error_page_path.c_str(), O_RDONLY);
    if (fd < 0)
        return response;
    std::string body;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        body.append(buffer, bytes_read);
    }
    close(fd);
    if (bytes_read < 0)
        return response;
    response.body = body;
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/html";
    return response;
}