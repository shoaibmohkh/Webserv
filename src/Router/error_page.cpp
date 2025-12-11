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