## 🚀 Parallel Webserv Work Plan — *Understand While Coding*

This plan is solid. The idea of **learning while implementing** is exactly right, and splitting the project this way lets **three people work in parallel with minimal blocking**.

For clarity, we’ll refer to the roles as **Partner 1 / Partner 2 / Partner 3**.

---

## 👨‍💻 Partner 1 — Networking & Event Loop (Pure Low-Level)

### 🎯 Goal

Own **everything related to networking and I/O**:

* Sockets
* Non-blocking I/O
* `poll` / `select` / `epoll`
* Buffers
* Timeouts

Partner 1 only deals with **raw bytes**:

> bytes in → bytes out
> No HTTP semantics. No parsing. No routing.

---

### (M) A.1 — TCP & OS Networking Foundations

**Topics**

* TCP 3-way handshake
* FIN / ACK connection teardown
* TCP reliability and retransmission basics
* File descriptors
* Kernel socket buffers (send/receive queues)
* Relationship between TCP and HTTP
* HTTP/1.0 non-persistent connections

**Keywords**

* `tcp three way handshake explained`
* `tcp connection teardown fin ack`
* `kernel socket buffer receive queue`
* `file descriptor linux`
* `tcp vs http relationship`
* `http1.0 non persistent connections`

---

### (M) A.2 — Basic Socket Programming

**Topics**

* `socket`, `bind`, `listen`, `accept`
* `connect` (for understanding only)
* `send` / `recv`
* Partial reads and writes
* Error handling: `EINTR`, `EAGAIN`, `EWOULDBLOCK`

**Keywords**

* `c socket programming guide`
* `socket bind listen accept tutorial`
* `tcp partial read write handling`
* `eagain ewouldblock nonblocking`
* `linux socket errors`

---

### (M) A.3 — Non-Blocking I/O

**Topics**

* `fcntl(fd, F_SETFL, O_NONBLOCK)`
* Why blocking is forbidden in this project
* Behavior of non-blocking `accept`, `read`, `write`
* Why slow clients break blocking servers
* Ensuring the event loop never stalls

**Keywords**

* `nonblocking sockets fcntl o_nonblock`
* `why nonblocking io for servers`
* `c++ network server nonblocking`

---

### (M) A.4 — I/O Multiplexing (poll / select / epoll)

**Topics**

* `pollfd` structure
* `POLLIN`, `POLLOUT`, `POLLHUP`, `POLLERR`
* Single-threaded event loop
* Adding and removing file descriptors
* When to read vs when to write
* Per-connection state machine
  *(READING_HEADERS, READING_BODY, WRITING, CLOSED)*

**Keywords**

* `poll c tutorial`
* `pollin pollout explanation`
* `event driven architecture c++`
* `single threaded nonblocking server loop`
* `state machine for network connections`

---

### (M) A.5 — Buffering Incoming Data

**Topics**

* TCP segmentation and fragmentation
* Buffering until `\r\n\r\n` (end of headers)
* Handling large request bodies (raw bytes only)
* Detecting disconnects (`recv == 0`)
* Queuing outgoing data
* Sending data in chunks using `POLLOUT`

**Keywords**

* `tcp fragmentation partial messages`
* `detect http header boundary crlf`
* `c++ rolling buffer design`
* `incremental read buffer`

---

### (MM) A.7 — Timeouts & Stress Resistance

**Topics**

* Connection timeouts
* Slowloris-style attacks (slow headers / slow body)
* Removing stuck clients from `poll`
* Basic stress testing with many connections

**Keywords**

* `tcp timeout handling server`
* `slowloris protection`
* `server connection timeout design`

---

### 📦 Interface for Partner 1

Partner 1 should expose something like:

```cpp
struct Connection {
    std::string read_buffer;
    std::string write_buffer;
    State state; // READING_HEADERS, READING_BODY, WRITING, CLOSED
};
```

**EventLoop responsibilities**

* Call a callback (Partner 2) when a full HTTP request is ready
* Accept a ready-made HTTP response string and send it
* No HTTP logic — only raw data movement

---

## 👨‍💻 Partner 2 — HTTP Protocol & Core Request/Response

### 🎯 Goal

Own the **HTTP brain**:

* Parsing requests
* Building responses
* Status codes
* Headers
* MIME types

Partner 2 does **not** care:

* How bytes arrive
* How bytes are sent

They only work with **strings and structures**.

---

### (M) B.1 — HTTP/1.0 Fundamentals

**Topics**

* Request line: `METHOD URI VERSION`
* Methods: GET, POST, DELETE (HEAD optional)
* Header format: `Key: Value`
* `Content-Length`
* `Connection: close`
* Essential status codes

**Keywords**

* `http 1.0 specification`
* `http request line example`
* `http status codes minimal`
* `content length http explanation`
* `http header parsing tutorial`

---

### (MM) B.3 — MIME Types / Content-Type

**Topics**

* Mapping file extensions to MIME types
* Why browsers care (`text/html`, `text/css`, `image/png`, …)

**Keywords**

* `http mime types mapping`
* `content type guessing webserver`

---

### (M) B.6 — HTTP Request Parsing

**Topics**

* Parse request line
* Parse headers into a map
* Validate fields (method, version)
* Extract body (Partner 1 delivers raw bytes)
* Build a `HttpRequest` object

**Keywords**

* `http request parser c++`
* `parse http headers key value`
* `content length body parsing`
* `http request line parsing`

---

### (M) B.8 — Response Generation

**Topics**

* Static file responses
* Status line + headers + blank line + body
* Always set `Content-Length`
* Error responses (4xx / 5xx)
* Responses for POST, DELETE, uploads, autoindex

**Keywords**

* `generate http response c++`
* `content length header build`
* `building http headers manually`

---

### (Optional) B.10 — Advanced HTTP/1.1

Only if you want to extend later:

* Keep-Alive
* Pipelining
* Caching
* Chunked responses

---

### 📦 Interface for Partner 2

Partner 2 defines **pure HTTP types**:

```cpp
class HttpRequest { ... };
class HttpResponse { ... };

HttpRequest parseHttpRequest(const std::string &raw);
std::string buildHttpResponse(const HttpResponse &res);
```

* Partner 1 can test with fake strings
* Partner 3 consumes these directly

---

## 👨‍💻 Partner 3 — Config, Routing, Filesystem & CGI

### 🎯 Goal

Own **server behavior**:

* Configuration
* Routing
* Filesystem access
* Autoindex
* Uploads
* CGI
* Error pages

---

### (MM) B.4 — NGINX Configuration Concepts

**Topics**

* `server` blocks
* Multiple ports
* `root`, `index`
* `autoindex`
* `client_max_body_size`
* `error_page`
* `return 301`

**Keywords**

* `nginx config server block explained`
* `nginx root vs alias difference`
* `nginx autoindex directory listing`
* `nginx error_page how it works`

---

### (M) B.5 — Configuration File Parser

**Topics**

* Tokenizer
* Recursive-descent parser
* `ServerConfig`
* `LocationConfig`
* Validation (duplicates, unknown directives)

**Keywords**

* `config file parser c++`
* `tokenizer vs parser`
* `recursive descent parser example`

---

### (M) B.7 — Routing Logic

**Topics**

* URI → best matching location
* Method allowed checks
* Filesystem path resolution
* File vs directory
* Autoindex HTML generation
* Upload handling
* Redirects (301 / 302)

**Keywords**

* `web server routing logic c++`
* `uri to filesystem path`
* `autoindex generate directory listing`

---

### (M) B.9 — CGI

**Topics**

* What CGI is
* Required environment variables
* `fork` + `execve` + pipes
* Writing request body to CGI stdin
* Reading CGI stdout
* Parsing CGI headers
* Converting output to `HttpResponse`

**Keywords**

* `cgi explained environment variables`
* `fork execve pipe example`
* `cgi stdout parsing`
* `cgi http header output example`

---

### Optional Extras for Partner 3

* Custom error pages
* Logging
* Passing timeout config to Partner 1

---

### 📦 Interface for Partner 3

Partner 3 sits **between Partner 1 and Partner 2**:

```cpp
HttpResponse handleRequest(const HttpRequest &req,
                           const ServerConfig &serverConfig);
```

Inside `handleRequest`:

* Select `LocationConfig`
* Check allowed methods
* Decide action: static / autoindex / upload / CGI / redirect / error
* Build a logical `HttpResponse`

Partner 2 serializes it.
Partner 1 sends it.

---

## 🧩 How Everyone Can Start Immediately

### Partner 1

* Build event loop and connections
* Send a hard-coded HTTP response initially

### Partner 2

* Implement and unit-test HTTP parsing and response building
* No networking required

### Partner 3

* Build config parser and routing logic
* Use fake filesystem data at first

---

## ✅ Agreement Required Early

All partners must agree on:

* `HttpRequest` fields
* `HttpResponse` fields
* `ServerConfig` and `LocationConfig` structures

Once agreed, everyone can work independently and plug everything together later.

