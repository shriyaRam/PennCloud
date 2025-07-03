#include "HttpResponse.h"
#include "utils.h"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <utility>

using namespace std;

HttpResponse::HttpResponse(int status_code, const std::string& page_name, const std::unordered_map<std::string, std::string>& template_values, string redirect){
    this->status_code = status_code;

    // Set the redirect location if the status code is 302
    if (status_code == 302) {
        redirect_location = redirect;
    }
    // Form the response body
    if (status_code != 302) {
        form_response_body(page_name, template_values);
    }
    // Form the response header
    form_response_header(response_body.length(), false);
}

HttpResponse::HttpResponse(int status_code, string file_data, string filename) {
    response_header = "HTTP/1.1 " + to_string(status_code) + " OK\r\n";
    response_header += "Content-Type: application/octet-stream\r\n";
    response_header += "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n";
    response_header += "Content-Length: " + to_string(file_data.length()) + "\r\n";
    response_header += "\r\n";
    response_body = file_data;
}

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
    response_header += close ? "Connection: close\r\n" : "Connection: Keep-Alive\r\n";

    // End of headers
    response_header += "\r\n";
}

void HttpResponse::form_response_body(const std::string& page_name, const std::unordered_map<std::string, std::string>& template_values) {

    if (status_code == 404) {
        response_body = "<html><body><h1>404 Not Found</h1></body></html>";
        return;
    }
    response_body = read_webpage(page_name); 
    // replace the template values (placeholders) with the actual values
    for (const auto& pair : template_values) {
        size_t pos = 0;
        while ((pos = response_body.find(pair.first, pos)) != std::string::npos) {
            response_body.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
}   

string HttpResponse::read_webpage(string page_key) {
    string directory = "../client/src/webpages/";
    string filename = directory + page_key + ".txt";
    
    // Open the file with POSIX open
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Unable to open the file: %s\n", filename.c_str());
        exit(1);
    }

    // Read the file contents
    string file_content;
    string buffer;
    while (true) {
        int bytes_read = wrapped_read(fd, &buffer);
        if (bytes_read == 0) {
            break;
        }
        file_content += buffer;
        buffer.clear();
    }

    // Close the file descriptor
    close(fd);
    return file_content;
}

void HttpResponse::set_cookie(std::string session_id){
    // replace the last "\r\n" with the Set-Cookie header
    string cookie_line = "Set-Cookie: session_id=" + session_id + "; HttpOnly; Secure; SameSite=Strict\r\n";
    response_header.replace(response_header.rfind("\r\n"), 2, cookie_line);
    response_header += "\r\n";
}

void HttpResponse::delete_cookie(){
    // replace the last "\r\n" with the Set-Cookie header
    string cookie_line = "Set-Cookie: session_id=; HttpOnly; Secure; SameSite=Strict; Max-Age=0\r\n";
    response_header.replace(response_header.rfind("\r\n"), 2, cookie_line);
    response_header += "\r\n";
}
