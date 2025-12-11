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
    std::string root = server.root;
    std::string loc = location.path;

    if (!root.empty() && root[root.size() - 1] != '/')
        root += "/";
    std::string clean_loc = loc;
    if (clean_loc.size() > 0 && clean_loc[0] == '/')
        clean_loc.erase(0,1);
    if (!clean_loc.empty() && clean_loc[clean_loc.size() - 1] != '/')
        clean_loc += "/";
    std::string remaining = uri.substr(loc.size());
    if (remaining.size() > 0 && remaining[0] == '/')
        remaining.erase(0,1);
    std::string full = root + clean_loc + remaining;
    while (full.find("//") != std::string::npos)
        full.erase(full.find("//"), 1);

    return full;
}


std::string Router::to_string(int value) const
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}
