#include "HttpConnection.h"
#include "utils.h"      // For wrapped_read() and wrapped_write()
#include "HttpResponse.h"
#include "RequestHandler.h"
#include <string>

using namespace std;

// Constructor
HttpConnection::HttpConnection(int fd, FrontendServer& server_ref) : comm_fd(fd), server(server_ref) {}

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
        request.set_body(body);
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
            Session session;

            // Validate session
            bool session_valid = validate_session(request, session);
            // if session is not valid, skip the request (the response is already sent in the validate_session function)
            if (!session_valid && request.to_validate_session()){
                continue;
            }
           
            // Handle the request
            RequestHandler handler(server);

            HttpResponse response = handler.handle_request(request, session);

            // Always update the session if it is valid
            if (session_valid && session.get_session_id() != ""){
                server.create_session(session.get_session_id(), session);
            }

            // Write the response to the client
            std::string response_str = response.get_response();
            if (request.get_method() == "HEAD"){
                response_str = response.get_response_header();    
            }

            write_response(response_str);

            // If "Connection: close" is specified in request headers, end connection
            if (request.connection_closed()){
                close_connection();
                return;
            }
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

bool HttpConnection::validate_session(HttpRequest& request, Session& session) {
    string session_id = request.get_session_id();
    if (!server.is_session_valid(session_id)) {
        session.set_valid(false);
        if (request.to_validate_session()){
            cout << "Session invalid. Redirecting to login...\n";
            HttpResponse response(302, "", {}, "/");
            write_response(response.get_response());
        }
        return false;
    }
    // Retrieve the session for use in the request handler
    session = server.get_session(session_id);
    session.set_valid(true);
    return true;
}
