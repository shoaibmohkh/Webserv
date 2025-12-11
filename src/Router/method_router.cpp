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

