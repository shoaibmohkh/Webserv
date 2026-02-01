/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:42:22 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:42:22 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cerrno>
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
    HTTPResponse apply_error_page(const ServerConfig& server_config, int status_code, HTTPResponse response) const;

    struct CgiSpawn
    {
        pid_t pid;
        int   fdIn;
        int   fdOut;
        CgiSpawn() : pid(-1), fdIn(-1), fdOut(-1) {}
    };

    bool spawn_cgi(const HTTPRequest& request,
                   const std::string& fullpath,
                   const LocationConfig& location_config,
                   CgiSpawn& outSpawn) const;

    HTTPResponse parse_cgi_response(const std::string& cgi_output) const;

    const ServerConfig&    find_server_config(const HTTPRequest& request) const;
    const LocationConfig*  find_location_config(const std::string &uri, const ServerConfig& server_config) const;
    std::string            final_path(const ServerConfig& server_config, const LocationConfig& location_config, const std::string& uri) const;
    bool                   is_cgi_request(const LocationConfig& location_config, const std::string& fullpath) const;

    std::string            method_to_string(HTTPMethod method) const;
    std::string            to_string(int value) const;
    std::string            to_string(size_t value) const;

    std::vector<std::string> build_cgi_environment(const HTTPRequest& request, const std::string& fullpath) const;

private:
    const Config& _config;

    bool        is_method_allowed(const LocationConfig& location_config, HTTPMethod method) const;
    std::string get_mime_type(const std::string& filepath) const;
    std::string read_file_binary(const std::string& filepath) const;

    HTTPResponse generate_autoindex_response(const std::string& path) const;
    // HTTPResponse handle_cgi_request(const HTTPRequest& request, const std::string& fullpath, const LocationConfig& location_config) const;
    HTTPResponse serve_static_file(const std::string& fullpath) const;

    HTTPResponse handle_post_request(const HTTPRequest& request,
                                     const LocationConfig& location_config,
                                     const ServerConfig& server_config,
                                     const std::string& fullpath) const;

    HTTPResponse handle_delete_request(const std::string& fullpath) const;
};

#endif
