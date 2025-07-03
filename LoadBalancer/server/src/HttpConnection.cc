#include "HttpConnection.h"
#include "utils.h"      // For wrapped_read() and wrapped_write()
#include "HttpResponse.h"
#include <string>

using namespace std;

// Retrieve the file descriptor
int HttpConnection::get_fd() const {
    return comm_fd;
}

// set the thread id
void HttpConnection::set_thread_id(pthread_t id) {
    thread_id = id;
}

// Check if the end of the HTTP header has been reached
bool HttpConnection::read_end_of_header() {
    return buffer_.find(kHeaderEnd) != std::string::npos;
}

// Read and parse the HTTP request
HttpRequest HttpConnection::read_request() {
    size_t end_of_header = buffer_.find(kHeaderEnd) + kHeaderEndLen;
    std::string header_str = buffer_.substr(0, end_of_header);
    buffer_.erase(0, end_of_header); // Remove the processed request from the buffer

    // Parse headers
    HttpRequest request(header_str);
    int content_length = request.get_content_length();
    if (content_length > 0) {
        // Read the request body
        while (buffer_.size() < content_length) {
            wrapped_read(comm_fd, &buffer_);
        }
        std::string body = buffer_.substr(0, content_length);
        buffer_.erase(0, content_length);
    }
    return request;
}

// Process the request and send the response
void HttpConnection::process_request() {
    while (true) {
        // Continuously read data until we have at least one complete request in buffer_
        while (!read_end_of_header()) {
            int bytes_read = wrapped_read(comm_fd, &buffer_);
            if (bytes_read < 0) {
                // If read fails or connection is closed, exit the loop
                std::cout << "Client disconnected or read error occurred\n";
                close_connection();
                return;
            }
        }
        // Process each complete request in buffer_
        while (read_end_of_header()) {
            
            HttpRequest request = read_request();
            // findout the server to which the request should be forwarded
            string server_id = lb_.get_next_server();
            HttpResponse response(302, "", {}, "http://" + server_id);
            write_response(response.get_response());
            // Close the connection after sending the response
            close_connection();
        }
    }
}

// Write the response to the client
void HttpConnection::write_response(std::string response) {
    wrapped_write(comm_fd, response);
}

// Close the connection by closing the file descriptor
void HttpConnection::close_connection() {
    close(comm_fd);
}
