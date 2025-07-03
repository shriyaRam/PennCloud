#include "HttpRequest.h"

using namespace std;

// Constructor to parse the request headers
HttpRequest::HttpRequest(std::string header_str) {

    raw_data = header_str;

    // read the method (GET/POST...etc.)
    size_t method_end = header_str.find(' ');
    method = header_str.substr(0, method_end);
    
    // read the path and query string
    size_t path_end = header_str.find(' ', method_end + 1);
    parse_uri(header_str.substr(method_end + 1, path_end - method_end - 1));
    
    // Parse the header string and populate the header_map
    size_t start = 0;
    size_t line_end = header_str.find("\r\n");
    while (line_end != std::string::npos) {
        std::string line = header_str.substr(start, line_end - start);
        size_t colon_pos = line.find(":");
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            header_map[key] = value;
        }
        start = line_end + 2;
        line_end = header_str.find("\r\n", start);
    }
}


// Parse the path and query string
void HttpRequest::parse_uri(string uri_str) {
    size_t query_start = uri_str.find("?");
    if (query_start != std::string::npos) {
        uri = uri_str.substr(0, query_start);
        std::string query = uri_str.substr(query_start + 1);
        size_t start = 0;
        size_t query_end = query.find("&");
        while (query_end != std::string::npos) {
            std::string param = query.substr(start, query_end - start);
            size_t equal_pos = param.find("=");
            if (equal_pos != std::string::npos) {
                std::string key = param.substr(0, equal_pos);
                std::string value = param.substr(equal_pos + 1);
                query_map[key] = value;
            }
            start = query_end + 1;
            query_end = query.find("&", start);
        }

        // Parse the last parameter
        std::string param = query.substr(start);
        size_t equal_pos = param.find("=");
        if (equal_pos != std::string::npos) {
            std::string key = param.substr(0, equal_pos);
            std::string value = param.substr(equal_pos + 1);
            query_map[key] = value;
        }
    } else {
        uri = uri_str;
    }
}

void HttpRequest::print_request() const {
    cout << raw_data << endl;
}

// Get the content length from the request headers
int HttpRequest::get_content_length() const {
    if (header_map.find("Content-Length") != header_map.end()) {
        return std::stoi(header_map.at("Content-Length"));
    }
    return 0;
}

// Check if connection: closed
bool HttpRequest::connection_closed(){
    if (header_map.find("Connection") != header_map.end() && header_map["Connection"] == "close" ) {
        return true;
    }
    return false;
}
