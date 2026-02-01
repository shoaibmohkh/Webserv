/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:19:17 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:40:41 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <cstdlib>
#include <csignal>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include "sockets/PollReactor.hpp"
#include "RouterByteHandler.hpp"
#include "config_headers/Tokenizer.hpp"
#include "config_headers/Parser.hpp"
#include "config_headers/Config.hpp"

#define DEFAULT_BACKLOG 128
#define DEFAULT_IDLE_TIMEOUT 30
#define DEFAULT_HEADER_TIMEOUT 10
#define DEFAULT_BODY_TIMEOUT 20
#define DEFAULT_MAX_HEADER_BYTES (17 * 1024)
#define DEFAULT_MAX_BODY_BYTES (1024ul * 1024ul * 1024ul + 1024ul * 1024ul)

static volatile sig_atomic_t g_stop = 0;

static void onSignal(int) {
    g_stop = 1;
}

static size_t extractMaxBodyBytes(const Config& cfg) {

    size_t mx = DEFAULT_MAX_BODY_BYTES;
    for (size_t i = 0; i < cfg.servers.size(); ++i) {
        long long v = (long long)cfg.servers[i].client_Max_Body_Size;
        if (v == 0) return 0;
        if (v > 0) {
            size_t sv = static_cast<size_t>(v);
            if (sv > mx) mx = sv;
        }
    }
    return mx;
}


static std::string readFileToString(const char* path) {
    std::ifstream in(path);
    if (!in.is_open()) throw std::runtime_error(std::string("Cannot open config file: ") + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static Config loadConfig(const char* confPath) {
    std::string input = readFileToString(confPath);
    Tokenizer t(input);
    std::vector<Token> tokens = t.tokenize();
    Parser p(tokens);
    Config cfg = p.parse();
    if (p.hasFatalError()) throw std::runtime_error("Invalid configuration (syntax error).");
    return cfg;
}

static std::string intToString(int n) {
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

static std::vector<int> extractListenPorts(const Config& cfg) {
    std::set<int> uniq;
    for (size_t i = 0; i < cfg.servers.size(); ++i) {
        int port = cfg.servers[i].port;
        if (port <= 0 || port > 65535) throw std::runtime_error("Invalid listen port in config: " + intToString(port));
        uniq.insert(port);
    }
    if (uniq.empty()) throw std::runtime_error("No listen ports found in config.");
    return std::vector<int>(uniq.begin(), uniq.end());
}

int main(int ac, char** av) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    const char* confPath = (ac >= 2) ? av[1] : "webserv.conf";
    if (ac > 2) {
        std::cerr << "Usage: " << av[0] << " [config_path]\n";
        std::cerr << "Example: " << av[0] << " webserv.conf\n";
        return 1;
    }

    try {
        Config cfg = loadConfig(confPath);
        if (cfg.servers.empty()) throw std::runtime_error("No server blocks found in config.");

        std::vector<int> ports = extractListenPorts(cfg);
        RouterByteHandler handler(confPath);
        size_t maxBody = extractMaxBodyBytes(cfg);

        PollReactor reactor(ports, DEFAULT_BACKLOG, DEFAULT_IDLE_TIMEOUT, DEFAULT_HEADER_TIMEOUT, DEFAULT_BODY_TIMEOUT, DEFAULT_MAX_HEADER_BYTES, maxBody, &handler);
        
        while (!g_stop) reactor.tickOnce();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

