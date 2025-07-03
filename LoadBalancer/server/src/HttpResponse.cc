#include "HttpResponse.h"
#include "utils.h"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <utility>

using namespace std;

// Constructor
HttpResponse::HttpResponse(int status_code, const std::string& page_name, const std::unordered_map<std::string, std::string>& template_values, string redirect){
    this->status_code = status_code;
    // Set the redirect location if the status code is 302
    if (status_code == 302) {
        redirect_location = redirect;
    }
    // Form the response header
    form_response_header(0, false);
}

// Form the response body
void HttpResponse::form_response_header(int body_length, bool close) {

    // Initialize the response header with HTTP version and status message
    response_header = "HTTP/1.1 " + to_string(status_code) + " ";

    // Set the status message based on the status code
    switch (status_code) {
        case 200:
            response_header += "OK";
            break;
        case 302:
            response_header += "Found";
            break;
        case 400:
            response_header += "Bad Request";
            break;
        case 404:
            response_header += "Not Found";
            body_length = 0;  // Ensure Content-Length is zero for 404
            break;
        default:
            response_header += "Unknown Status";
            break;
    }
    response_header += "\r\n";

    // Set common headers
    response_header += "Content-Type: text/html; charset=UTF-8\r\n";
    response_header += "Content-Length: " + to_string(body_length) + "\r\n";
    // Add Location header for redirects
    if (status_code == 302 && !redirect_location.empty()) {
        response_header += "Location: " + redirect_location + "\r\n";
    }

    // Connection header
    response_header += "Connection: close\r\n";

    // End of headers
    response_header += "\r\n";
}
