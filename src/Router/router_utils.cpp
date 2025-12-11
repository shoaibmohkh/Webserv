/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   router_utils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 20:46:00 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 20:46:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Router_headers/Router.hpp"

std::string Router::final_path(const ServerConfig& server,
                               const LocationConfig& location,
                               const std::string& uri) const
{
    std::string loc = location.path;
    std::string root = location.root.empty() ? server.root : location.root;
    std::string remaining;

    if (loc == "/")
    {
        remaining = uri;
    }
    else if (uri.compare(0, loc.length(), loc) == 0)
    {
        remaining = uri.substr(loc.length());
        if (remaining.empty())
            remaining = "/";
    }
    else
        remaining = uri;
    if (!root.empty() && root[root.length() - 1] != '/')
        root += "/";
    if (!remaining.empty() && remaining[0] == '/')
        remaining = remaining.substr(1);
    std::string full = root + remaining;
    for (size_t pos = full.find("//"); pos != std::string::npos; pos = full.find("//"))
        full.erase(pos, 1);

    return full;
}

std::string Router::to_string(int value) const
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}
