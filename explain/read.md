# Webserv Project - Complete Overview

## Project Summary
Webserv is a **HTTP/1.0 Web Server** implementation written in C++98. It's designed to parse configuration files, route HTTP requests, and serve static files with advanced features like CGI support, file uploads, auto-indexing, and error page handling.

---

## Architecture Overview

### Three Main Components:

```
┌─────────────────────────────────────────────────────────┐
│                   WEBSERV ARCHITECTURE                   │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  1. CONFIG PARSING (Tokenizer → Parser)                  │
│     └─ Reads webserv.conf and creates Config objects    │
│                                                           │
│  2. HTTP TYPES (HttpTypes & Http headers)                │
│     └─ Defines HTTPRequest/HTTPResponse structures      │
│                                                           │
│  3. ROUTING ENGINE (Router)                              │
│     └─ Handles HTTP requests and generates responses    │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

---

## COMPLETED FEATURES ✅

### 1. Configuration Parsing System

#### **Tokenizer** (`include/config_headers/Tokenizer.hpp`)
- **Purpose**: Lexical analysis of webserv.conf file
- **Functionality**:
  - Reads raw config file content
  - Tokenizes into: `WORD`, `L_BRACE` ({), `R_BRACE` (}), `SEMICOLON` (;)
  - Tracks line numbers for error reporting
  - Skips whitespace and comments
- **Status**: ✅ **COMPLETE**

#### **Parser** (`include/config_headers/Parser.hpp`)
- **Purpose**: Syntactic analysis and config object creation
- **Implements Parsing For**:
  - Server blocks (`listen`, `root`, `index`, `server_name`, `client_max_body_size`, `error_page`)
  - Location blocks (`path`, `autoindex`, `allow_methods`, `upload_enable`, `upload_store`, `cgi_extension`, `return` redirects)
  - Nested location configurations per server
- **Error Handling**: Basic error messages for syntax violations
- **Status**: ✅ **COMPLETE**

#### **Config Data Structures** (`include/config_headers/Config.hpp`)
Three-level hierarchy:
```
Config
  └─ ServerConfig[]
      ├─ port (8080)
      ├─ root (./www)
      ├─ index (index.html)
      ├─ server_name (mysite)
      ├─ client_max_body_size (max upload size)
      ├─ error_pages (map of status codes to error page paths)
      └─ LocationConfig[]
          ├─ path (/images)
          ├─ autoindex (on/off)
          ├─ allowMethods (GET, POST, DELETE)
          ├─ uploadEnable (on/off)
          ├─ uploadStore (upload directory)
          ├─ cgiExtensions (map of file extensions to interpreters)
          └─ return (redirect code and path)
```
- **Status**: ✅ **COMPLETE**

### 2. HTTP Types System

#### **HttpTypes.hpp**
- **HTTPMethod enum**: GET, POST, DELETE, UNKNOWN
- **HTTPRequest struct**:
  - `port`: Extracted from Host header
  - `method`: HTTP method
  - `uri`: Request path (/images/cat.png)
  - `version`: HTTP/1.1 or HTTP/1.0
  - `host`: Extracted hostname only
  - `body`: Request body for POST/PUT
  - `headers`: All HTTP headers as key-value pairs
- **HTTPResponse struct**:
  - `status_code`: HTTP status (200, 404, 500, etc.)
  - `reason_phrase`: Status message (OK, Not Found, etc.)
  - `body`: Response body
  - `headers`: Response headers
- **Status**: ✅ **COMPLETE**

### 3. Router System (`include/Router_headers/Router.hpp`)

#### **Core Routing Logic**
- **Main Entry Point**: `handle_route_Request(const HTTPRequest& request)`
  - Validates server configuration exists (500 error if none)
  - Finds matching server by Host header
  - Locates appropriate location block for URI
  - Handles method validation
  - Routes to appropriate handler

#### **Implemented Request Handlers**

1. **GET Requests** (`serve_static_file`)
   - Serves static files (HTML, CSS, JS, images, etc.)
   - MIME type detection
   - Content-Length calculation
   - Binary file support

2. **POST Requests** (`handle_post_request`)
   - File upload handling when `upload_enable on`
   - Saves uploaded files to `upload_store` directory
   - Respects `client_max_body_size` limit
   - Returns appropriate response

3. **DELETE Requests** (`handle_delete_request`)
   - Deletes files from server
   - Validates permissions

4. **CGI Support** (`handle_cgi_request`)
   - Executes CGI scripts (Python, etc.)
   - Sets up CGI environment variables
   - Parses CGI output back to HTTP response
   - Supports `cgi_extension .py /usr/bin/python3` directive

5. **Auto-indexing** (`generate_autoindex_response`)
   - Generates HTML directory listings when `autoindex on`
   - Lists files and directories in readable format

#### **Helper Functions**
- `find_server_config()`: Matches server by Host header
- `find_location_config()`: Matches location block by URI
- `is_method_allowed()`: Validates allowed methods
- `is_cgi_request()`: Detects if file is CGI script
- `apply_error_page()`: Returns custom error pages from config
- `get_mime_type()`: Determines Content-Type header
- `read_file_binary()`: Reads files as binary data
- `parse_cgi_response()`: Converts CGI output to HTTPResponse
- `build_cgi_environment()`: Creates environment for CGI execution

- **Status**: ✅ **COMPLETE** (Core routing functional)

#### **Error Handling**
- **500 Internal Server Error**: No servers configured
- **404 Not Found**: File/location doesn't exist, custom error pages supported
- **403 Forbidden**: Method not allowed for location
- **Custom Error Pages**: Maps status codes to HTML files per server

### 4. Router Implementations

#### **Method Router** (`src/Router/method_router.cpp`)
- Handles different HTTP methods
- Validates method against location configuration

#### **CGI Router** (`src/Router/cgi_router.cpp`)
- Executes CGI scripts
- Manages subprocess execution
- Captures and parses output

#### **File Handling** (`src/Router/files_handeling.cpp`)
- File reading/writing operations
- File upload processing
- Upload directory management

#### **Auto-indexing** (`src/Router/autoindex.cpp`)
- Generates HTML directory listings
- Formats file information

#### **Error Page Handling** (`src/Router/error_page.cpp`)
- Loads and returns custom error pages
- Falls back to default error messages

#### **Router Utilities** (`src/Router/router_utils.cpp`)
- Helper functions for routing logic
- Path manipulation
- MIME type mapping

### 5. Logging System

#### **Logger** (`src/logger/Logger.cpp` & `Logger.hpp`)
- Console output for debugging
- Constructor/destructor logging
- Message formatting
- **Note**: Marked for removal in future versions

- **Status**: ✅ **IMPLEMENTED** (Basic)

---

## MISSING FEATURES ❌

### 1. **Network/Socket Layer (CRITICAL)**
- ❌ TCP server socket creation
- ❌ Connection accepting (accept())
- ❌ Non-blocking socket I/O
- ❌ select() or poll() for multiplexing
- ❌ Socket event loop
- ❌ Connection management (keep-alive vs close)
- ❌ Timeout handling
- ❌ TLS/SSL support

**Impact**: The router can't actually receive HTTP requests or send responses over the network. Currently, `HTTPRequest` objects would need to be created externally.

### 2. **HTTP Parsing (CRITICAL)**
- ❌ Raw HTTP request parsing from bytes
- ❌ Request line parsing (GET /path HTTP/1.1)
- ❌ Header parsing and extraction
- ❌ Request body handling (multipart/form-data)
- ❌ Query string parsing
- ❌ URL decoding
- ❌ Chunked transfer encoding
- ❌ Keep-Alive header processing

**Impact**: Can't convert raw TCP bytes into `HTTPRequest` structures. The framework expects pre-parsed requests.

### 3. **Response Generation**
- ❌ Proper HTTP status line generation
- ❌ Automatic header formatting (Date, Server, Content-Length)
- ❌ Content-Type negotiation
- ❌ Cache-Control headers
- ❌ ETag/Last-Modified support
- ❌ Conditional GET (304 Not Modified)
- ❌ Response serialization to bytes

**Impact**: Can generate `HTTPResponse` objects but can't format them as valid HTTP responses to send over network.

### 4. **Server Integration (CRITICAL)**
- ❌ Main server loop (`main()` function)
- ❌ Event-driven architecture
- ❌ Worker thread/process management
- ❌ Configuration loading at startup
- ❌ Graceful shutdown mechanism
- ❌ SIGINT/SIGTERM signal handling
- ❌ Port binding and listening
- ❌ Client connection handling

**Impact**: No executable entry point to run the server.

### 5. **Advanced Features**
- ❌ **Reverse Proxy**: No proxy request forwarding
- ❌ **Load Balancing**: No upstream server distribution
- ❌ **WebSocket Support**: No WebSocket upgrade handling
- ❌ **HTTP/1.1 Persistent Connections**: Only HTTP/1.0 non-persistent
- ❌ **CORS Headers**: No cross-origin handling
- ❌ **Compression**: No gzip/deflate support
- ❌ **Range Requests**: No partial content (206)
- ❌ **Basic Auth**: No authentication mechanisms

### 6. **Performance & Reliability**
- ❌ Concurrent request handling
- ❌ Connection pooling
- ❌ Memory leak prevention
- ❌ Buffer overflow protection
- ❌ Slow client handling
- ❌ Resource limits enforcement
- ❌ Logging to files
- ❌ Performance monitoring

### 7. **Testing**
- ❌ Unit tests
- ❌ Integration tests
- ❌ Stress testing
- ❌ Test file cleanup in Makefile
- **Note**: `src/testfile.cpp` contains commented-out test code

---

## Current Functionality vs. Missing Pieces

```
┌──────────────────────────────────────────────────────────┐
│                    EXECUTION FLOW                         │
├──────────────────────────────────────────────────────────┤
│                                                            │
│  ✅ COMPLETED (Data Processing)                          │
│  ┌─────────────────────────────────────────────────────┐ │
│  │ Config File → Tokenizer → Parser → Config Objects  │ │
│  │ HTTPRequest → Router → HTTPResponse                │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                            │
│  ❌ MISSING (Network I/O)                                │
│  ┌─────────────────────────────────────────────────────┐ │
│  │ Socket → Raw Bytes → HTTP Parser → HTTPRequest    │ │
│  │ HTTPResponse → Formatter → Raw Bytes → Socket     │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                            │
│  ❌ MISSING (Application Glue)                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │ main() → Socket Setup → Event Loop → Threading    │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                            │
└──────────────────────────────────────────────────────────┘
```

---

## What Can Currently Be Done

1. ✅ **Load and parse a configuration file**
   ```cpp
   Tokenizer tokenizer(fileContent);
   Parser parser(tokenizer.tokenize());
   Config config = parser.parse();
   ```

2. ✅ **Create a Router and route a request** (if HTTPRequest is manually created)
   ```cpp
   Router router(config);
   HTTPRequest req = {/* manually populated */};
   HTTPResponse resp = router.handle_route_Request(req);
   ```

3. ✅ **Handle static files, uploads, CGI, and directory listing** (routing logic)

4. ✅ **Generate proper HTTP responses** (status codes, headers, bodies)

---

## What's Needed to Make It a Real Server

### Phase 1: HTTP Communication (CRITICAL)
1. Raw HTTP request parser (parse bytes → HTTPRequest)
2. HTTP response serializer (HTTPResponse → bytes)
3. Socket creation and management
4. Non-blocking I/O with select/poll

### Phase 2: Server Infrastructure (CRITICAL)
1. Main server loop
2. Event-driven request handling
3. Configuration loading at startup
4. Graceful shutdown

### Phase 3: Robustness
1. Error handling and validation
2. Memory safety
3. Connection timeouts
4. Logging system

### Phase 4: Features (Optional for HTTP/1.0)
1. Keep-Alive support
2. Better error messages
3. Performance optimization

---

## File Structure Summary

| Component | Files | Status |
|-----------|-------|--------|
| **Config Parsing** | Tokenizer.hpp/cpp, Parser.hpp/cpp, Config.hpp | ✅ Complete |
| **HTTP Types** | HttpTypes.hpp, Http.hpp | ✅ Complete |
| **Router** | Router.hpp/cpp, method_router.cpp, cgi_router.cpp, file_handling.cpp, autoindex.cpp, error_page.cpp, router_utils.cpp | ✅ Complete |
| **Logging** | Logger.hpp/cpp | ✅ Basic |
| **Main Server** | ❌ Missing | ❌ Not Started |
| **Socket Layer** | ❌ Missing | ❌ Not Started |
| **HTTP Parser** | ❌ Missing | ❌ Not Started |
| **Tests** | testfile.cpp (commented) | ⚠️ Partial |

---

## Conclusion

**Webserv is 60% data-processing engine, 0% network server.**

The project has a solid foundation for:
- ✅ Configuration management
- ✅ Request routing logic
- ✅ Response generation

But it's missing the critical network layer that would connect these components together and actually serve HTTP clients.

To make this a functional web server, the next team member should focus on:
1. **Implementing the socket layer** (TCP server, non-blocking I/O)
2. **Creating HTTP request/response parsers** (bytes ↔ structs)
3. **Building the main server loop** (event-driven architecture)

Once these are in place, all the existing routing and configuration logic will work as designed.
