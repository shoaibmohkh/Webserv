*This project has been created as part of the 42 curriculum by sal-kawa, ikhalil, eaqrabaw.*

# Webserv

This is our HTTP server written in C++98. We built it from scratch to learn how web servers actually work under the hood.

## Description

We wrote this server to understand how HTTP works. It can serve websites, handle file uploads, and run CGI scripts (like Python). The whole thing uses `poll()` for handling multiple connections without blocking.

### What it does

- Listens on multiple ports at the same time
- Serves static files (HTML, CSS, JS, images, etc.)
- Handles GET, POST, and DELETE requests
- Uploads files to the server
- Runs CGI scripts based on file extension
- Shows directory listings when you want it
- Custom error pages (404, 403, 500, etc.)
- HTTP redirections (301)
- Limits on request body size

## Instructions

### What you need

- A C++ compiler (we use `c++` which is usually clang++)
- Linux or macOS
- Make

### How to compile

```bash
make        # builds the server
make clean  # removes object files
make fclean # removes everything (object files + executable)
make re     # rebuilds from scratch
```

We compile with `-Wall -Wextra -Werror -std=c++98`

### How to run

```bash
./webserv                    # uses webserv.conf by default
./webserv your_config.conf   # use your own config file
```

### Configuration file

The config file looks similar to NGINX. Here's an example:

```nginx
server {
    listen 8080;
    server_name localhost;
    root test_root;
    index index.html;
    client_max_body_size 20000000;

    error_page 404 /errors/404.html;
    error_page 403 /errors/403.html;
    error_page 500 /errors/500.html;

    location / {
        allow_methods GET POST DELETE;
        autoindex off;
    }

    location /files {
        allow_methods GET;
    }

    location /listing_dir {
        allow_methods GET;
        autoindex on;
    }

    location /old {
        return 301 /new_location;
    }

    location /cgi {
        allow_methods GET POST;
        cgi_extension .py /usr/bin/python3;
    }

    location /uploads {
        allow_methods GET POST;
        upload_enable on;
        upload_store /path/to/uploads;
    }

    location /delete_zone {
        allow_methods DELETE GET;
    }
}
```

### Config options

| Option | What it does |
|--------|--------------|
| `listen` | Port number |
| `server_name` | Name of the server |
| `root` | Where your files are |
| `index` | Default file for directories |
| `client_max_body_size` | Max size for request body (in bytes) |
| `error_page` | Custom error page for a status code |
| `allow_methods` | Which methods are allowed (GET, POST, DELETE) |
| `autoindex` | Show directory listing (on/off) |
| `upload_enable` | Allow file uploads (on/off) |
| `upload_store` | Where to save uploaded files |
| `cgi_extension` | File extension and interpreter for CGI |
| `return` | Redirect to another URL |

### Testing

```bash
# simple GET
curl http://localhost:8080/

# POST some data
curl -X POST -d "name=test" http://localhost:8080/

# upload a file
curl -X POST -F "file=@myfile.txt" http://localhost:8080/uploads/

# delete a file
curl -X DELETE http://localhost:8080/delete_zone/somefile.txt
```

Or just open your browser and go to `http://localhost:8080/`

---

## Resources

- [RFC 1945 – HTTP/1.0](https://datatracker.ietf.org/doc/html/rfc1945)
- [NGINX Documentation](https://nginx.org/)
- [RFC 3875 – The Common Gateway Interface (CGI)](https://www.ietf.org/rfc/rfc3875)
- [Linux Socket Programming in C/C++](https://www.linuxhowtos.org/C_C++/socket.htm)
- [I/O Multiplexing: select and poll](https://notes.shichao.io/unp/ch6/)

---

## AI Usage

- Guided us on how to get started and what concepts we needed to learn
- Helped us build a roadmap for the project
- Generated additional test cases to improve our testing coverage

---

## Project Structure

```
webserv/
├── Makefile
├── README.md
├── include/
│   ├── RouterByteHandler.hpp
│   ├── config_headers/
│   │   ├── Config.hpp
│   │   ├── Parser.hpp
│   │   └── Tokenizer.hpp
│   ├── HTTP/
│   │   ├── HttpRequest.hpp
│   │   ├── HttpResponse.hpp
│   │   └── http10/
│   │       ├── Http10Parser.hpp
│   │       └── Http10Serializer.hpp
│   ├── Router_headers/
│   │   └── Router.hpp
│   └── sockets/
│       ├── IByteHandler.hpp
│       ├── ICgiHandler.hpp
│       ├── ListenPort.hpp
│       ├── NetChannel.hpp
│       ├── NetUtil.hpp
│       └── PollReactor.hpp
├── src/
│   ├── main.cpp
│   ├── config/
│   │   ├── Tokenizer.cpp
│   │   ├── location_parser.cpp
│   │   ├── parser.cpp
│   │   └── server_parser.cpp
│   ├── HTTP/
│   │   ├── Http10Parser.cpp
│   │   └── Http10Serializer.cpp
│   ├── Router/
│   │   ├── Router.cpp
│   │   ├── RouterByteHandler.cpp
│   │   ├── autoindex.cpp
│   │   ├── cgi_router.cpp
│   │   ├── error_page.cpp
│   │   ├── files_handeling.cpp
│   │   ├── method_router.cpp
│   │   └── router_utils.cpp
│   └── sockets/
│       ├── ListenPort.cpp
│       ├── NetChannel.cpp
│       ├── NetUtil.cpp
│       └── PollReactor.cpp
└── test_root/
    ├── index.html
    ├── router_test.conf
    ├── autoindex_dir/
    │   └── file1.txt
    ├── cgi/
    │   ├── a.py
    │   ├── error.py
    │   ├── hello.txt
    │   ├── loop.py
    │   ├── q.py
    │   └── test.py
    ├── delete_zone/
    ├── errors/
    │   ├── 403.html
    │   ├── 404.html
    │   └── 500.html
    ├── files/
    │   ├── a.txt
    │   ├── index.html
    │   ├── todel.txt
    │   └── assets/
    │       └── style.css
    ├── listing_dir/
    │   ├── file1.txt
    │   └── file2.txt
    ├── private_dir/
    │   └── private.txt
    ├── uploads/
    └── with_index_dir/
        └── index2.html
```

---

## Team

| Name | Intra |
|------|-------|
| Shoaib | sal-kawa |
| Ismail | ikhalil |
| Eyad | eaqrabaw |

---

Made at 42 Amman
