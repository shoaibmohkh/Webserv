#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "HttpParser.hpp"
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
        "BADREQUEST /oops HTTP/1.0\n\r\n"
        "\r\n"
    );

    return requests;
}


int main() {
    std::vector<std::string> mock_requests = get_mock_requests();

    HttpParser req1(mock_requests[3]);
    req1.parseRequest();
}
