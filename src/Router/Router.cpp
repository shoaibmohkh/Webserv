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


// **Detailed Explanation of handle_route_Request Logic :

// STEP 1 — No Servers Loaded (500)
// Situation (Bad config)

// Config file:
// (no server blocks at all)

// Request :
// GET / HTTP/1.1
// Host: localhost
// What your code sees :
// _config.servers.empty() == true
// What you return

// Response :
// HTTP/1.1 500 Internal Server Error
// Content-Type: text/plain
// Content-Length: 58
// 500 Internal Server Error: No server configurations available.

// Why?
// Because the server cannot function without a server block.

// --------------------------------------------------------------

// STEP 2 — Server Selection :
// find_server_config(request)

// Assume config:
// server {
//     listen 8080;
//     server_name example.com;
// }

// Request :
// GET / HTTP/1.1
// Host: example.com
// What happens

// Result :
// You select this server block.

// No response yet — just choosing context.

// --------------------------------------------------------------

// STEP 3 — Location Not Found (404)

// Config:
// location /images { ... }

// Request:
// GET /videos/movie.mp4 HTTP/1.1
// What happens
// find_location_config("/videos/movie.mp4") → NULL

// Response:
// HTTP/1.1 404 Not Found
// Content-Type: text/plain
// Content-Length: 13
// 404 Not Found

// Why?
// No matching location → resource does not exist.

// --------------------------------------------------------------

// STEP 4 — Method Not Allowed (405)

// Config:
// location /upload {
//     allow_methods GET;
// }

// Request:
// POST /upload HTTP/1.1

// What happens
// is_method_allowed(...) == false

// Response:
// HTTP/1.1 405 Method Not Allowed
// Allow: GET
// Content-Type: text/plain
// Content-Length: 22
// 405 Method Not Allowed


// Key detail:
// Allow header must exist

// --------------------------------------------------------------

// STEP 5 — Redirect (return)

// Config:
// location /old {
//     return 301 /new;
// }

// Request:
// GET /old HTTP/1.1

// What happens ??
// No file access
// No stat()
// Immediate redirect

// Response:
// HTTP/1.1 301 Redirect
// Location: /new
// Content-Length: 20
// Redirecting to /new

// --------------------------------------------------------------

// STEP 6 — Final Path Resolution

// Config:
// root /var/www/site;
// location /images {
//     root /var/www/media;
// }

// Request:
// GET /images/logo.png HTTP/1.1

// final_path() result :
// /var/www/media/logo.png

// Why?
// /images removed
// location root applied

// --------------------------------------------------------------

// STEP 7 — POST Request (Upload)

// Config:
// location /upload {
//     upload_enable on;
//     upload_path /uploads;
// }

// Request:
// POST /upload/file.txt HTTP/1.1
// Content-Length: 11
// hello world

// What happens ??
// handle_post_request(...)

// File created :
// /uploads/file.txt

// Response:
// HTTP/1.1 201 Created
// Content-Length: 7
// Content-Type: text/plain
// Created


// Important:
// File may not exist before
// POST handled before stat()

// --------------------------------------------------------------

// STEP 8 — File Not Found (404)
// stat() fails

// Request:
// GET /nofile.html HTTP/1.1

// Response:
// HTTP/1.1 404 Not Found
// Content-Type: text/plain
// Content-Length: 13
// 404 Not Found

// ---------------------------------------------------------------

// STEP 9 — Directory Handling:
//-----------------------------------
// Case A — Autoindex ON
// Config:
// location /files {
//     autoindex on;
// }

// Directory:
// /files/
//  ├ a.txt
//  └ b/
// Request:
// GET /files HTTP/1.1

// Response (simplified) :
// php-template
// HTTP/1.1 200 OK
// Content-Type: text/html
// <html>
// <a href="a.txt">a.txt</a>
// <a href="b/">b/</a>
// </html>

//-----------------------------------
// Case B — Index Exists

// Config:
// index index.html;

// Directory:
// /site/
//  └ index.html

// Request:
// GET /site HTTP/1.1

// Response:
// HTTP/1.1 200 OK
// Content-Type: text/html
// Content-Length: ...
// (index.html content)

//-----------------------------------
// Case C — Forbidden
// No index
// Autoindex OFF

// Response:
// HTTP/1.1 403 Forbidden
// Content-Type: text/plain
// Content-Length: 13
// 403 Forbidden

// --------------------------------------------------------------

// STEP 10 — Regular File :

//-----------------------------------
// Case - CGI

// Request:
// GET /test.py HTTP/1.1

// CGI output:
// Content-Type: text/html
// <h1>Hello</h1>

// Response:
// HTTP/1.1 200 OK
// Content-Type: text/html
// Content-Length: 14
// <h1>Hello</h1>


//-----------------------------------
// Case - Static File

// Request:
// GET /hello.txt HTTP/1.1

// File content:
// hello

// Response:
// HTTP/1.1 200 OK
// Content-Type: text/plain
// Content-Length: 5
// hello

// --------------------------------------------------------------

// STEP 11 — DELETE :

// Delete file Request:
// DELETE /file.txt HTTP/1.1

// Response:
// HTTP/1.1 200 OK
// Content-Length: 7
// Deleted

// Delete Directory Request:
// DELETE /folder HTTP/1.1

// Response:
// HTTP/1.1 403 Forbidden


HTTPResponse Router::handle_route_Request(const HTTPRequest& request) const
{
    HTTPResponse response;
    if (_config.servers.empty()) //step one
    {
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.body = "500 Internal Server Error: No server configurations available.";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(_config.servers[0], response.status_code, response);
    }

    const ServerConfig &server_config = find_server_config(request); //step two
    const LocationConfig* location_config = find_location_config(request.uri, server_config);
    if (!location_config) //step three
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(server_config, response.status_code, response);
    }
    if (!is_method_allowed(*location_config, request.method)) //step four
    {
        // For every request, after we find the location, we check if the HTTP method is allowed
        // there. If not, we return 405 Method Not Allowed and include an Allow header listing
        // the permitted methods. If the location doesn’t specify methods, we allow all supported
        // ones by default.
        //-------------------------------------------
        // Example 1 — POST not allowed
        // Config:
        // location /images {
        //     allow_methods GET;
        // }

        // Request:
        // POST /images/logo.png HTTP/1.1

        // What happens?
        // Location exists ✔
        // File exists ✔
        // Method POST ❌

        // Response:
        // HTTP/1.1 405 Method Not Allowed
        // Allow: GET
        // Content-Type: text/plain
        // Content-Length: 22
        // 405 Method Not Allowed
        //-------------------------------------------
        // Example 2 — DELETE not allowed, multiple allowed methods
        // Config:
        // location /files {
        //     allow_methods GET POST;
        // }

        // Request:
        // DELETE /files/a.txt HTTP/1.1

        // Response:
        // HTTP/1.1 405 Method Not Allowed
        // Allow: GET, POST
        // Content-Type: text/plain
        // Content-Length: 22
        // 405 Method Not Allowed
        //-------------------------------------------
        // Example 3 — allowMethods empty (all allowed)
        // Config:
        // location /any {
        //     # no allow_methods
        // }

        // Request:
        // DELETE /any/file.txt HTTP/1.1

        // Result:
        // ✔️ Allowed
        // ➡️ Continues routing, no 405.
        //-------------------------------------------
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
    if (location_config->returnCode != 0) //step five
    {
        response.status_code = location_config->returnCode;
        response.reason_phrase = "Redirect";
        response.headers["Location"] = location_config->returnPath;
        response.body = "Redirecting to " + location_config->returnPath;
        response.headers["Content-Length"] = to_string(response.body.size());

        return apply_error_page(server_config, response.status_code, response);
    }
    std::string fullpath = final_path(server_config, *location_config, request.uri); //step six

    struct stat sb;
    if (request.method == HTTP_POST) //step seven
    {
        response = handle_post_request(request, *location_config, server_config, fullpath);
        return apply_error_page(server_config, response.status_code, response);
    }
    if (stat(fullpath.c_str(), &sb) != 0) //step eight
    {
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.body = "404 Not Found";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(server_config, response.status_code, response);
    }

    if (S_ISDIR(sb.st_mode)) //step nine
    {
        if (location_config->autoindex == true) //step nine - case A
        {
            response = generate_autoindex_response(fullpath);
            return apply_error_page(server_config, response.status_code, response);
        }
        if (!server_config.index.empty())//step nine - case B
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
        response.status_code = 403; //step nine - case C
        response.reason_phrase = "Forbidden";
        response.body = "403 Forbidden";
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";

        return apply_error_page(server_config, response.status_code, response);
    }
    if (S_ISREG(sb.st_mode)) //step ten
    {
        if (is_cgi_request(*location_config, fullpath)) //step ten - case CGI
        {
            response = handle_cgi_request(request, fullpath, *location_config);
            return apply_error_page(server_config, response.status_code, response);
        }
        else
        {
            response = serve_static_file(fullpath); //step ten - case Static File
            return apply_error_page(server_config, response.status_code, response);
        }
    }
    // if (request.method == HTTP_POST) 
    // {
    //     response = handle_post_request(request, *location_config, server_config, fullpath);
    //     return apply_error_page(server_config, response.status_code, response);
    // }
    if (request.method == HTTP_DELETE) //step eleven
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

// Find_location_config Explanation:
// **This function searches for the best matching location configuration for a given URI
// within a specific server configuration. It iterates through the location blocks
// defined in the server configuration and finds the most specific match for the
// requested URI.
//-------------------------------------------
// **The code (simplified meaning) :
// best_match = NULL
// best_length = 0

// for each location in server:
//     if uri starts with location.path:
//         if location.path is longer than current best:
//             best_match = location

// if nothing matched:
//     return "/" location if exists

// return best_match
//-------------------------------------------
// Example 1 — Simple match

// Config:
// server {
//     location /images { }
//     location /api { }
// }

// Request:
// GET /images/logo.png

// Step-by-step in code:
// /images matches /images/logo.png
// /api does NOT match
// best_match = /images

// Returned value:
// LocationConfig { path = "/images" }
//-------------------------------------------
// Example 2 — Longest prefix wins (VERY IMPORTANT)
// Config:
// server {
//     location /images { }
//     location /images/icons { }
//     location / { }
// }
// Request:
// GET /images/icons/logo.png

// Matching results :
// Location	    Matches?	Length
// /images	      yes	      7
// /images/icons  yes	      13
//    /	          yes	      1
// ￼
// Code decision:
// best_length = 13
// best_match = "/images/icons"

// Returned value:
// LocationConfig { path = "/images/icons" }
//-------------------------------------------
// Example 3 — No exact match → fallback /
// Config:
// server {
//     location / { }
//     location /api { }
// }

// Request:
// GET /unknown/path

// Code behavior:
// /api ❌
// / matches everything
// best_match == NULL
// root_location != NULL

// Returned:
// LocationConfig { path = "/" }
//-------------------------------------------
// Example 4 — No locations at all

// Config:
// server {
//     (no location blocks)
// }

// Request:
// GET /

// Code:
// if (server_config.locations.empty())
//     return NULL;

// Result:
// NULL

// Which later becomes:
// 404 Not Found
//-------------------------------------------
// Example 5 — URI shorter than location path

// Config:
// location /images/icons { }

// Request:
// GET /images

// Code:
// if (uri.size() < loc_path.size())
//     continue;

// Result:
// ❌ No match


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

// Find_server_config Explanation:
// **This function selects the appropriate server configuration for an incoming HTTP request
// based on the request's Host header(server_name) and port number. 
// It implements the logic to handle

//-----------------------------------------------------------
// Example 1 — Single server only
// Config:
// server {
//     listen 8080;
// }

// Request:
// GET / HTTP/1.1
// Host: anything

// Code path:
// candidates = servers where port == request.port


// Only one server → return it.

//-----------------------------------------------------------
// Example 2 — Multiple servers, same port, different names
// Config:
// server {
//     listen 8080;
//     server_name example.com;
// }

// server {
//     listen 8080;
//     server_name test.com;
// }
//--------------------------------------
// Request A:
// GET / HTTP/1.1
// Host: example.com

// Code flow:
// Both servers match port 8080
// host = "example.com"
// Compare against server_name
// Match found

// Returned:
// server_name = example.com

//--------------------------------------
// Request B
// GET / HTTP/1.1
// Host: unknown.com

// Code flow:
// Port matches both
// Host does not match any server_name
// Return first server on that port

// Returned:
// server_name = example.com

// Correct default behavior.

//-----------------------------------------------------------
// Example 3 — Host header missing
// Request:
// GET / HTTP/1.1
// (no Host header)

// Code:
// if (host.empty())
//     return *candidates[0];

// Why??
// HTTP/1.0 does not require Host.

// Correct fallback.

//-----------------------------------------------------------
// Example 4 — No server matches the port
// Config:
// server {
//     listen 8080;
// }

// Request:
// GET / HTTP/1.1
// (port 9090)

// Code:
// if (candidates.empty())
//     return _config.servers[0];

// Result:
// Fallback to default server.

// Evaluators expect this.

//-----------------------------------------------------------
// Example 5 — Case-insensitive matching
// Config:
// server_name Example.COM;

// Request:
// Host: example.com

// Code:
// tolower(host)
// tolower(server_name)

// Result:
// Match ✔
// Correct per HTTP spec.

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