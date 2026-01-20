// // #include "Tokenizer.hpp"
// // #include <fstream>
// // #include <sstream>

// // int main() {
// //     std::ifstream file("webserv.conf");
// //     if (!file) {
// //         std::cerr << "Could not open config file.\n";
// //         return 1;
// //     }

// //     std::stringstream buffer;
// //     buffer << file.rdbuf();

// //     Tokenizer t(buffer.str());
// //     std::vector<Token> tokens = t.tokenize();

// //     for (size_t i = 0; i < tokens.size(); ++i) {
// //         std::cout << tokens[i].line << ": ";

// //         if (tokens[i].type == WORD)
// //             std::cout << "[WORD] " << tokens[i].value;
// //         else if (tokens[i].type == L_BRACE)
// //             std::cout << "[{]";
// //         else if (tokens[i].type == R_BRACE)
// //             std::cout << "[}]";
// //         else if (tokens[i].type == SEMICOLON)
// //             std::cout << "[;]";

// //         std::cout << "\n";
// //     }
// // }

// // #include "../include/config_headers/Tokenizer.hpp"
// // #include "../include/config_headers/parser.hpp"
// // #include <fstream>
// // #include <sstream>

// // void printLocation(const LocationConfig &loc)
// // {
// //     std::cout << "    Location path: " << loc.path << "\n";
// //     std::cout << "      autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
// //     std::cout << "      upload_enable: " << (loc.uploadEnable ? "on" : "off") << "\n";
// //     std::cout << "      upload_store: " << loc.uploadStore << "\n";

// //     std::cout << "      allow_methods: ";
// //     for (size_t i = 0; i < loc.allowMethods.size(); ++i)
// //         std::cout << loc.allowMethods[i] << " ";
// //     std::cout << "\n";

// //     std::cout << "      return: " << loc.returnCode << " " << loc.returnPath << "\n";

// //     std::cout << "      cgi_extensions:\n";
// //     for (std::map<std::string, std::string>::const_iterator it = loc.cgiExtensions.begin();
// //          it != loc.cgiExtensions.end(); ++it)
// //     {
// //         std::cout << "        " << it->first << " -> " << it->second << "\n";
// //     }
// // }

// // void printServer(const ServerConfig &s)
// // {
// //     std::cout << "Server:\n";
// //     std::cout << "  listen: " << s.port << "\n";
// //     std::cout << "  client_max_body_size: " << s.client_Max_Body_Size << "\n";
// //     std::cout << "  root: " << s.root << "\n";
// //     std::cout << "  index: " << s.index << "\n";

// //     std::cout << "  error_pages:\n";
// //     for (std::map<int, std::string>::const_iterator it = s.error_Pages.begin();
// //          it != s.error_Pages.end(); ++it)
// //     {
// //         std::cout << "    " << it->first << " -> " << it->second << "\n";
// //     }

// //     std::cout << "  locations:\n";
// //     for (size_t i = 0; i < s.locations.size(); ++i)
// //         printLocation(s.locations[i]);
// // }

// // int main()
// // {
// //     std::ifstream file("test_root/router_test.conf");
// //     if (!file)
// //     {
// //         std::cerr << "Could not open config file.\n";git@github.com:shoaibmohkh/Webserv.git
// //         return 1;
// //     }

// //     std::stringstream buffer;
// //     buffer << file.rdbuf();

// //     Tokenizer tokenizer(buffer.str());
// //     std::vector<Token> tokens = tokenizer.tokenize();

// //     Parser parser(tokens);
// //     Config conf = parser.parse();

// //     for (size_t i = 0; i < conf.servers.size(); ++i)
// //     {
// //         std::cout << "\n===========================\n";
// //         std::cout << " SERVER #" << i + 1 << "\n";
// //         std::cout << "===========================\n";
// //         printServer(conf.servers[i]);
// //     }

// //     return 0;
// // }
// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <sys/stat.h>
// #include <vector>   // <-- add
// #include <map>      // (likely already pulled by your headers, but safe)

// #include "Router.hpp"
// #include "Parser.hpp"
// #include "Tokenizer.hpp"

// HTTPMethod str_to_method(const std::string &m) {
//     if (m == "GET") return HTTP_GET;
//     if (m == "POST") return HTTP_POST;
//     if (m == "DELETE") return HTTP_DELETE;
//     return HTTP_UNKNOWN;
// }

// // Helper: set vector<char> from std::string (C++98)
// static void set_body_from_string(std::vector<char> &dst, const std::string &src) {
//     dst.assign(src.begin(), src.end());
// }

// // Helper: print vector<char> safely (binary-safe)
// static void print_body_vector(const std::vector<char> &body) {
//     if (body.empty())
//         return;
//     std::cout.write(&body[0], body.size());
// }

// HTTPRequest make_request(const std::string &method, const std::string &uri, const std::string &body = "") {
//     HTTPRequest req;
//     req.method = str_to_method(method);
//     req.uri = uri;
//     req.version = "HTTP/1.1";
//     req.headers["Host"] = "localhost";
//     req.host = "localhost";
//     req.port = 8080;

//     // was: req.body = body;
//     set_body_from_string(req.body, body);

//     return req;
// }

// void print_response(const HTTPResponse &res) {
//     // std::cout << "STATUS: " << res.status_code << " " << res.reason_phrase << "\n";
//     // for (std::map<std::string,std::string>::const_iterator it = res.headers.begin();
//     //      it != res.headers.end(); ++it)
//     // {
//     //     std::cout << it->first << ": " << it->second << "\n";
//     // }

//     // std::cout << "BODY:\n";
//     // was: std::cout << res.body << "\n";
//     print_body_vector(res.body);
//     // std::cout << "\n";

//     // std::cout << "---------------------------------------------\n";
// }

// bool file_exists(const std::string &path) {
//     struct stat st;
//     return (stat(path.c_str(), &st) == 0);
// }

// int main(int argc, char **argv) {

//     std::string configPath;

//     if (argc > 1)
//         configPath = argv[1];
//     else
//         configPath = "test_root/router_test.conf";

//     // std::cout << "Loading config file: " << configPath << std::endl;

//     if (!file_exists(configPath)) {
//         // std::cerr << "ERROR: Config file not found: " << configPath << "\n";
//         return 1;
//     }

//     std::ifstream file(configPath.c_str());
//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     file.close();

//     Tokenizer tokenizer(buffer.str());
//     std::vector<Token> tokens = tokenizer.tokenize();

//     Parser parser(tokens);
//     Config cfg = parser.parse();

//     Router router(cfg);

//     // ---------------------------------------------------------
//     // BASIC ROUTING TESTS
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 1: ROOT INDEX =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/")));

//     std::cout << "===== TEST 2: STATIC FILE =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/files/a.txt")));

//     std::cout << "===== TEST 3: FILE NOT FOUND =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/files/unknown.txt")));

//     std::cout << "===== TEST 4: METHOD NOT ALLOWED (DELETE on /files) =====\n";
//     print_response(router.handle_route_Request(make_request("DELETE","/files/a.txt")));

//     std::cout << "===== TEST 5: PRIVATE DIRECTORY (403 Forbidden) =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/private_dir/")));

//     std::cout << "===== TEST 6: AUTOINDEX DIRECTORY =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/listing_dir/")));

//     std::cout << "===== TEST 7.0: REDIRECT (/old → /new_location) =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/old")));
//     // std::cout << "===== TEST 7.1: image autoindex_dir =====\n";
//     // print_response(router.handle_route_Request(make_request("GET","/autoindex_dir/file2.png")));

//     // ---------------------------------------------------------
//     // CGI TEST
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 8: CGI TEST =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/cgi/test.py")));

//     // ---------------------------------------------------------
//     // POST UPLOAD
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 9: UPLOAD (POST to /uploads) =====\n";
//     print_response(router.handle_route_Request(make_request("POST","/uploads","UPLOADED CONTENT")));

//     // ---------------------------------------------------------
//     // DELETE
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 10: DELETE (delete_zone/removeme.txt) =====\n";
//     print_response(router.handle_route_Request(make_request("DELETE","/delete_zone/removeme.txt")));

//     // ---------------------------------------------------------
//     // NORMAL 404 (router)
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 11: FULL 404 TEST =====\n";
//     print_response(router.handle_route_Request(make_request("GET","/does/not/exist")));

//     // ---------------------------------------------------------
//     // CUSTOM 404
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 12: CUSTOM 404 PAGE =====\n";
//     HTTPResponse r404;
//     r404.status_code = 404;
//     r404.reason_phrase = "Not Found";
//     // was: r404.body = "ORIGINAL 404";
//     set_body_from_string(r404.body, "ORIGINAL 404");
//     r404 = router.apply_error_page(cfg.servers[0], 404, r404);
//     print_response(r404);

//     // ---------------------------------------------------------
//     // CUSTOM 403
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 13: CUSTOM 403 PAGE =====\n";
//     HTTPResponse r403;
//     r403.status_code = 403;
//     r403.reason_phrase = "Forbidden";
//     // was: r403.body = "ORIGINAL 403";
//     set_body_from_string(r403.body, "ORIGINAL 403");
//     r403 = router.apply_error_page(cfg.servers[0], 403, r403);
//     print_response(r403);

//     // ---------------------------------------------------------
//     // CUSTOM 500
//     // CORRECTED TEST — now simulates a real server 500
//     // ---------------------------------------------------------
//     std::cout << "===== TEST 14: CUSTOM 500 PAGE (Simulated) =====\n";
//     HTTPResponse r500;
//     r500.status_code = 500;
//     r500.reason_phrase = "Internal Server Error";
//     // was: r500.body = "Original internal error before replacement";
//     set_body_from_string(r500.body, "Original internal error before replacement");
//     r500 = router.apply_error_page(cfg.servers[0], 500, r500);
//     print_response(r500);

//     return 0;
// }
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <vector>

#include "sockets/PollReactor.hpp"
#include "sockets/NetUtil.hpp"

#include "RouterByteHandler.hpp"

#define DEFAULT_PORTS "8080"
#define DEFAULT_BACKLOG 128

#define DEFAULT_IDLE_TIMEOUT   30
#define DEFAULT_HEADER_TIMEOUT 10
#define DEFAULT_BODY_TIMEOUT   20

#define DEFAULT_MAX_HEADER_BYTES (16 * 1024)
#define DEFAULT_MAX_BODY_BYTES   (2 * 1024 * 1024)

static volatile sig_atomic_t g_stop = 0;

static void onSignal(int)
{
    g_stop = 1;
}

int main(int ac, char** av)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    const char* portsArg  = (ac >= 2) ? av[1] : DEFAULT_PORTS;
    const char* confPath  = (ac >= 3) ? av[2] : "webserv.conf";

    std::vector<int> ports;
    if (!parsePortsList(portsArg, ports))
    {
        std::cerr << "Usage: " << av[0] << " [ports_csv] [config_path]\n";
        std::cerr << "Example: " << av[0] << " 8080,8081 webserv.conf\n";
        return 1;
    }

    try
    {
        RouterByteHandler handler(confPath);

        PollReactor reactor(ports,
                            DEFAULT_BACKLOG,
                            DEFAULT_IDLE_TIMEOUT,
                            DEFAULT_HEADER_TIMEOUT,
                            DEFAULT_BODY_TIMEOUT,
                            DEFAULT_MAX_HEADER_BYTES,
                            DEFAULT_MAX_BODY_BYTES,
                            &handler);

        while (!g_stop)
            reactor.tickOnce();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
