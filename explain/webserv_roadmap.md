# Webserv Roadmap (HTTP/1.0 Focus)

A complete study roadmap for both partners, including detailed topics,
keywords, and importance levels.

**Legend**\
- **(M)** Mandatory for HTTP/1.0 & Webserv\
- **(MM)** Medium importance (useful but not required)\
- **(U)** Unnecessary for HTTP/1.0 (knowledge only)

------------------------------------------------------------------------

# ================================

# PARTNER A --- NETWORKING & EVENT LOOP

# ================================

Partner A handles **raw bytes**, network events, and all low-level
socket logic.

------------------------------------------------------------------------

## A.1 --- TCP & OS Networking Foundations (M)

### Detailed topics

-   TCP 3-way handshake\
-   TCP connection teardown (FIN/ACK)\
-   TCP reliability: acknowledgments, retransmission\
-   OS file descriptors\
-   Socket buffer internals (recv/send queues)\
-   HTTP/1.0 is non-persistent by default

### Keywords

    tcp three way handshake explained
    tcp connection teardown fin ack
    kernel socket buffer receive queue
    file descriptor linux
    tcp vs http relationship
    http1.0 non persistent connections

------------------------------------------------------------------------

## A.2 --- Basic Socket Programming (M)

### Detailed topics

-   socket(AF_INET, SOCK_STREAM, 0)\
-   Binding IP + port (bind)\
-   listen(backlog)\
-   accept() returning new client fd\
-   connect() behavior (for understanding)\
-   send() & recv() low-level behavior\
-   Handling partial reads/writes\
-   EINTR / EAGAIN / EWOULDBLOCK

### Keywords

    c socket programming guide
    socket bind listen accept tutorial
    tcp partial read write handling
    eagain ewouldblock nonblocking
    linux socket errors

------------------------------------------------------------------------

## A.3 --- Non-blocking I/O (M)

### Detailed topics

-   fcntl(fd, F_SETFL, O_NONBLOCK)\
-   Why blocking is forbidden in Webserv\
-   Behavior of non-blocking accept/read/write\
-   How slow clients break blocking servers\
-   Ensuring the event loop never stalls

### Keywords

    nonblocking sockets fcntl o_nonblock
    why nonblocking io for servers
    c++ network server nonblocking

------------------------------------------------------------------------

## A.4 --- I/O Multiplexing (poll/select/epoll) (M)

### Detailed topics

-   pollfd structure\
-   POLLIN, POLLOUT, POLLHUP, POLLERR\
-   Single-thread event loop design\
-   Registering/removing file descriptors\
-   When to read / when to write\
-   Connection state machines

### Keywords

    poll c tutorial
    pollin pollout explanation
    event driven architecture c++
    single threaded nonblocking server loop
    state machine for network connections

------------------------------------------------------------------------

## A.5 --- Buffering Incoming Data (M)

### Detailed topics

-   TCP segmentation (data arrives in chunks)\
-   Detecting end of headers: `\r\n\r\n`\
-   Buffering incomplete data\
-   Handling large request bodies\
-   Detecting client disconnect (0 bytes read)\
-   Chunked send using POLLOUT readiness

### Keywords

    tcp fragmentation partial messages
    detect http header boundary crlf
    c++ rolling buffer design
    incremental read buffer

------------------------------------------------------------------------

## A.6 --- Chunked Transfer Decoding (U)

HTTP/1.0 **does not use** chunked transfer.\
Skip unless you want extra knowledge.

### Keywords

    chunked transfer encoding http1.1

------------------------------------------------------------------------

## A.7 --- Timeouts & Stress Resistance (MM)

### Detailed topics

-   Timeout logic\
-   Slowloris-style attacks\
-   Handling stalled clients\
-   Stress testing behavior

### Keywords

    tcp timeout handling server
    slowloris protection
    server connection timeout design

------------------------------------------------------------------------

# ================================

# PARTNER B --- HTTP, CONFIG, ROUTING, RESPONSES, CGI

# ================================

Partner B handles **interpreting the bytes**, HTTP logic, configuration,
routing, responses, and CGI.

------------------------------------------------------------------------

## B.1 --- HTTP/1.0 Fundamentals (M)

### Detailed topics

-   Request line: `METHOD URI VERSION`\
-   Allowed HTTP/1.0 methods: GET, POST, HEAD (optional)\
-   DELETE (not in HTTP/1.0, but required by project)\
-   Header format\
-   Content-Length\
-   Connection: close\
-   Essential status codes

### Keywords

    http 1.0 specification
    http request line example
    http status codes minimal
    content length http explanation
    http header parsing tutorial

------------------------------------------------------------------------

## B.2 --- HTTP/1.1 Features (U)

Unnecessary for HTTP/1.0: - Host header (optional in 1.0)\
- Chunked transfer\
- Automatic keep-alive

### Keywords

    http1.1 chunked transfer
    host header http
    keep alive http

------------------------------------------------------------------------

## B.3 --- MIME Types / Content-Type (MM)

Not required by HTTP/1.0 but browsers expect it.

### Keywords

    http mime types mapping
    content type guessing webserver

------------------------------------------------------------------------

## B.4 --- NGINX Configuration Concepts (MM)

Useful for designing your own config parser.

### Study

-   server blocks\
-   multiple ports\
-   root, index\
-   autoindex\
-   client_max_body_size\
-   upload store\
-   error_page\
-   return 301 (redirection)

### Keywords

    nginx config server block explained
    nginx root vs alias difference
    nginx autoindex directory listing
    nginx error_page how it works

------------------------------------------------------------------------

## B.5 --- Configuration File Parser (M)

### Detailed topics

-   Tokenization\
-   Detecting braces `{}`\
-   Parsing directives\
-   Building configuration structures\
-   Inheritance (server → location)\
-   Validating invalid configs

### Keywords

    config file parser c++
    tokenizer vs parser
    recursive descent parser example
    parsing directives config file

------------------------------------------------------------------------

## B.6 --- HTTP Request Parsing (M)

### Detailed topics

-   Parsing request line\
-   Parsing headers into a map\
-   Validating mandatory fields\
-   Parsing body after Partner A buffers it\
-   Constructing a `Request` object

### Keywords

    http request parser c++
    parse http headers key value
    content length body parsing
    http request line parsing

------------------------------------------------------------------------

## B.7 --- Routing Logic (M)

### Detailed topics

-   URI → location block\
-   Allowed method checks\
-   Filesystem path resolution\
-   Directory vs file\
-   Autoindex implementation\
-   Redirection\
-   Upload handling

### Keywords

    web server routing logic c++
    uri to filesystem path
    autoindex generate directory listing
    http method allowed check

------------------------------------------------------------------------

## B.8 --- Response Generation (M)

### Detailed topics

-   Static file responses\
-   Building full HTTP/1.0 responses\
-   Content-Length & Content-Type\
-   Error pages\
-   POST upload responses\
-   DELETE responses\
-   Autoindex HTML

### Keywords

    generate http response c++
    content length header build
    building http headers manually
    directory listing html c++

------------------------------------------------------------------------

## B.9 --- CGI (M)

### Detailed topics

-   CGI concept\
-   Environment variables\
-   fork() + execve()\
-   Pipes for communication\
-   Writing request body to CGI\
-   Reading CGI output\
-   Parsing CGI headers\
-   Integrating with Response builder

### Keywords

    cgi explained environment variables
    fork execve pipe example
    cgi stdout parsing
    cgi http header output example

------------------------------------------------------------------------

## B.10 --- Advanced HTTP/1.1 Behavior (U)

Skip these unless for extra knowledge: - Keep-Alive logic\
- Pipelining\
- Chunked responses\
- Caching headers

### Keywords

    http keep alive mechanism
    http caching headers
