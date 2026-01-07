#include <iostream>
#include <string>
#include <vector>
#include <map>

// ------------------------
// Mock Partner 1
// ------------------------
std::vector<std::string> get_mock_requests() {
    std::vector<std::string> requests;

    // GET request
    requests.push_back(
        "GET /index.html HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "User-Agent: curl/7.79.1\r\n"
        "Accept: */*\r\n"
        "\r\n"
    );

    // POST request with body
    requests.push_back(
        "POST /upload HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World"
    );

    // DELETE request
    requests.push_back(
        "DELETE /delete_me.txt HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    // Malformed request (for testing 400)
    requests.push_back(
        "BADREQUEST /oops HTTP/1.0\r\n"
        "\r\n"
    );

    return requests;
}

// ------------------------
// Your parser interface (mock example)
// ------------------------
struct HTTPRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::string body;
    std::map<std::string, std::string> headers;
};

// Example parser stub
HTTPRequest parse_http_request(const std::string &raw) {
    HTTPRequest req;
    // --- minimal parsing just for demonstration ---
    size_t pos = raw.find("\r\n");
    std::string request_line = raw.substr(0, pos);
    size_t method_end = request_line.find(' ');
    size_t uri_end = request_line.find(' ', method_end + 1);

    req.method = request_line.substr(0, method_end);
    req.uri = request_line.substr(method_end + 1, uri_end - method_end - 1);
    req.version = request_line.substr(uri_end + 1);

    // Headers & body parsing would go here
    return req;
}

int main() {
    std::vector<std::string> mock_requests = get_mock_requests();

    for (size_t i = 0; i < mock_requests.size(); ++i) {
        std::cout << "----- Processing Mock Request #" << (i + 1) << " -----\n";
        HTTPRequest req = parse_http_request(mock_requests[i]);
        std::cout << "Method: " << req.method << "\n";
        std::cout << "URI: " << req.uri << "\n";
        std::cout << "Version: " << req.version << "\n";
        std::cout << "---------------------------------------------\n\n";
    }

    return 0;
}
