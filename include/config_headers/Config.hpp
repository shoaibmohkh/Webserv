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
        std::string                        root;              // Root for this location 
        std::vector<std::string>           allowMethods;      // Allowed methods (GET, POST, DELETE)
        std::map<std::string, std::string> cgiExtensions;     // CGI (.py, /usr/bin/python3)

        LocationConfig() : returnCode(0), uploadEnable(false), autoindex(false) {}
};

// server {
//     server_name mysite;
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
        std::string                              server_name;          // Server name (mysite)
        std::map<int, std::string>               error_Pages;          // Error pages (404, /errors/404.html)
        std::vector<LocationConfig>              locations;            // Location configurations

        ServerConfig() : port(80), client_Max_Body_Size(0), root("./www"), index("index.html"), server_name("default") {}
};

// server {
//     listen 8080;
//     server_name mysite;
//     root ./site1;
// }
// server {
//     listen 9090;
//     server_name api;
//     root ./apiroot;
// }
// GlobalConfig
//    ├── ServerConfig #1
//    │       ├── LocationConfig /images
//    │       ├── LocationConfig /upload
//    │       └── ...
//    └── ServerConfig #2
//            ├── LocationConfig /api
//            ├── LocationConfig /static
//            └── ...
class Config {
    public:
        std::vector<ServerConfig> servers; // List of server configurations
};

// full example for config file
// server {
//     listen 8080;                      ==>     ServerConfig::port
//     root ./www;                       ==>     ServerConfig::root
//     index index.html;                 ==>     ServerConfig::index
//     client_max_body_size 1000000;     ==>     ServerConfig::client_Max_Body_Size
//     error_page 404 /errors/404.html;  ==>     ServerConfig::error_Pages[404] = returnPath::/errors/404.html

//     location /images {
//         autoindex on;                       ==>     LocationConfig::autoindex
//         allow_methods GET POST;             ==>    LocationConfig::allowMethods ["GET", "POST"]
//         upload_enable on;                   ==>    LocationConfig::uploadEnable
//         upload_store /var/www/uploads;      ==>    LocationConfig::uploadStore
//         cgi_extension .py /usr/bin/python3; ==>    LocationConfig::cgiExtensions[".py"] = "/usr/bin/python3";
//         return 301 /new_images;             ==>    LocationConfig::returnCode = 301; LocationConfig::returnPath = "/new_images";
//     }

//     location /upload {
//         autoindex off;                 ==>     LocationConfig::autoindex
//         allow_methods POST;            ==>    LocationConfig::allowMethods ["POST"]
//         upload_enable on;              ==>    LocationConfig::uploadEnable
//         upload_store /var/www/uploads; ==>    LocationConfig::uploadStore
//     }
// }
//  *** server.locations = {
//          LocationConfig (path="/images", autoindex=true, allowMethods={"GET","POST"}, uploadEnable=true, uploadStore="/var/www/uploads", cgiExtensions={".py":"/usr/bin/python3"}, returnCode=301, returnPath="/new_images"), // LocationConfig for /images
//          LocationConfig (path="/upload", autoindex=false, allowMethods={"POST"}, uploadEnable=true, uploadStore="/var/www/uploads")  // LocationConfig for /upload
//     };

#endif