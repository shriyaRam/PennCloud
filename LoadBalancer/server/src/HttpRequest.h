#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>           // For std::string
#include <unordered_map>    // For std::unordered_map
#include <iostream>         // For std::cout, std::endl

using namespace std;

class HttpRequest {
private:
    std::string method; // Stores the HTTP method (GET, POST, etc.)
    std::string uri; // Stores the URI
    std::unordered_map<std::string, std::string> header_map; // Stores the header key-value pairs
    std::unordered_map<std::string, std::string> query_map; // Stores query key-value pairs

    std::string raw_data;

public:
    // Constructor to parse the request headers
    HttpRequest(std::string header_str);

    // Print the request headers and body
    void print_request() const;

    // Getters
    int get_content_length() const;
    string get_uri() const { return uri; }
    string get_method() const { return method; }
    std::unordered_map<std::string, std::string> get_query_map() const { return query_map; }

    // Parse the path and query string
    void parse_uri(string uri_str);

    // Check if Connection: close
    bool connection_closed();
};

#endif // HTTP_REQUEST_H
