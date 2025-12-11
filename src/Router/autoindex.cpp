/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   autoindex.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 20:49:02 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 20:49:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

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
