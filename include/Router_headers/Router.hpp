/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 02:25:33 by marvin            #+#    #+#             */
/*   Updated: 2025/12/11 02:25:33 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../HttpTypes.hpp"
#include "../config_headers/Config.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>



class Router
{
    public:
        Router(const Config& config);
        ~Router();
        HTTPResponse handle_route_Request(const HTTPRequest& request) const;
    private:
        const Config& _config;
        const ServerConfig &find_server_config(const HTTPRequest& request) const;
        const LocationConfig* find_location_config(const std::string &uri, const ServerConfig& server_config) const;
        bool is_method_allowed(const LocationConfig& location_config, HTTPMethod method) const;
        std::string method_to_string(HTTPMethod method) const;
        std::string to_string(int value) const;
        std::string final_path(const ServerConfig& server_config, const LocationConfig& location_config, const std::string& uri) const;
        HTTPResponse generate_autoindex_response(const std::string& path) const;
        HTTPResponse handle_cgi_request(const HTTPRequest& request, const std::string& fullpath, const LocationConfig& location_config) const;
        HTTPResponse serve_static_file(const std::string& fullpath) const;
        std::string get_mime_type(const std::string& filepath) const;
        std::string read_file_binary(const std::string& filepath) const;
        bool is_cgi_request(const LocationConfig& location_config, const std::string& fullpath) const;
        HTTPResponse parse_cgi_response(const std::string& cgi_output) const;
        std::vector<std::string> build_cgi_environment(const HTTPRequest& request, const std::string& fullpath) const;
};


#endif