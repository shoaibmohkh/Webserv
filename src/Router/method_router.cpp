/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   method_router.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 20:44:23 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 20:44:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

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

// Example handling POST request:
// Config:
// server {
//     root /var/www/site;
// }
// location /upload {
//     upload_enable on;
//     upload_store uploads;
// }

// Request:
// POST /upload/hello.txt HTTP/1.1
// Content-Length: 5
// hello

// File Created:
// /var/www/site/uploads/hello.txt

// File Content:
// hello

// Response:
// HTTP/1.1 201 Created
// Content-Type: text/plain
// Content-Length: 39
// 201 Created: File uploaded successfully.

HTTPResponse Router::handle_post_request(const HTTPRequest& request, const LocationConfig& location_config, const ServerConfig& server_config, const std::string& fullpath) const
{
    if (location_config.uploadEnable == false)
    {
        HTTPResponse response;
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Uploads are not enabled for this location.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    // Example Config:
    // server {
    //     root /var/www/site;
    // }
    //
    // location /upload {
    //     upload_enable on;
    //     upload_store uploads;
    // }
    //
    // Result:
    // upload_path = "/var/www/site/uploads"
    std::string upload_path =  server_config.root + "/" + location_config.uploadStore;
    // Then you ensure it ends with /:
    // /var/www/site/uploads/
    // This prevents broken paths like:
    // /var/www/site/uploadsfile.txt
    if (!upload_path.empty() && upload_path[upload_path.size() - 1] != '/')
        upload_path += '/';
    // Example Request:
    // POST /upload/photo.png HTTP/1.1
    // fullpath:
    // /var/www/site/upload/photo.png
    // Extracted filename:
    // photo.png
    std::string filename;
    size_t last_slash = fullpath.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < fullpath.size())
        filename = fullpath.substr(last_slash + 1);
    // Case: No Filename in URL
    // Request:
    // POST /upload HTTP/1.1

    // No filename after /upload.

    // Your fallback:
    // filename = "upload_1700000000.bin";

    // ✔ This is very good
    // ✔ Prevents overwrite
    // ✔ Prevents empty filename
    if (filename.empty())
        filename = "upload_" + to_string(time(NULL)) + ".bin";
    std::string final_upload_path = upload_path + filename;
    int fd = open(final_upload_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); // Permissions: rw-r--r--
    if (fd < 0)
    {
        HTTPResponse response;
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to open upload path.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    ssize_t bytes_written = write(fd, &request.body[0], request.body.size()); // Write Request Body to File
    close(fd);
    if (bytes_written < 0)
    {
        HTTPResponse response;
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to write to upload path.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    HTTPResponse response;
    response.status_code = 201;
    response.reason_phrase = "Created";
    response.set_body("201 Created: File uploaded successfully.");
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/plain";
    return response;
}

HTTPResponse Router::handle_delete_request(const std::string& fullpath) const
{
    struct stat sb;
    if (stat(fullpath.c_str(), &sb) != 0)
    {
        HTTPResponse response;
        response.status_code = 404;
        response.reason_phrase = "Not Found";
        response.set_body("404 Not Found: File does not exist.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    if (S_ISDIR(sb.st_mode))
    {
        HTTPResponse response;
        response.status_code = 403;
        response.reason_phrase = "Forbidden";
        response.set_body("403 Forbidden: Cannot delete a directory.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    if (unlink(fullpath.c_str()) < 0)
    {
        if (errno == EACCES || errno == EPERM)
        {
            HTTPResponse response;
            response.status_code = 403;
            response.reason_phrase = "Forbidden";
            response.set_body("403 Forbidden: Permission denied.");
            response.headers["Content-Length"] = to_string(response.body.size());
            response.headers["Content-Type"] = "text/plain";
            return response;
        }
        HTTPResponse response;
        response.status_code = 500;
        response.reason_phrase = "Internal Server Error";
        response.set_body("500 Internal Server Error: Unable to delete the file.");
        response.headers["Content-Length"] = to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    HTTPResponse response;
    response.status_code = 200;
    response.reason_phrase = "OK";
    response.set_body("200 OK: File deleted successfully.");
    response.headers["Content-Length"] = to_string(response.body.size());
    response.headers["Content-Type"] = "text/plain";
    return response;
}