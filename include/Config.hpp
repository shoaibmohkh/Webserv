/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 16:53:49 by marvin            #+#    #+#             */
/*   Updated: 2025/12/10 16:53:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <vector>

// location /images {
//     autoindex on;
//     allow_methods GET POST;
//     upload_enable on;
//     upload_store /var/www/uploads;
//     cgi_extension .py /usr/bin/python3;
//     return 301 /new_images;
// }

class LocationConfig {
    public:
        int                                returnCode;        // redirect code (301), 0 if none
        bool                               uploadEnable;      // Upload enable on/off
        bool                               autoindex;         // Autoindex on/off
        std::string                        path;              // Location path (/images)
        std::string                        returnPath;        // redirect path (/new_images)
        std::string                        uploadStore;       // Upload storage path (/var/www/uploads)
        std::vector<std::string>           allowMethods;      // Allowed methods (GET, POST, DELETE)
        std::map<std::string, std::string> cgiExtensions;     // CGI (.py, /usr/bin/python3)
};

// server {
//     listen 8080;
//     root ./www;
//     index index.html;
//     client_max_body_size 1000000;
//     error_page 404 /errors/404.html;
// }

class ServerConfig {
    public:
        int                                      port;                 // Port to listen on (8080)
        int                                      client_Max_Body_Size; // Max body size (1000000)
        std::string                              root;                 // Document root (./www)
        std::string                              index;                // Index file (index.html)
        std::map<int, std::string>               error_Pages;          // Error pages (404, /errors/404.html)
};

#endif