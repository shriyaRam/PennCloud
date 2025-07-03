#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <pthread.h>    // For pthread_t
#include <string>       // For std::string
#include <unistd.h>     // For close()
#include "HttpRequest.h"
#include "Session.h"
#include "FrontendServer.h"
class HttpConnection {
private:
    int comm_fd;               // File descriptor for the connection
    pthread_t thread_id;       // Thread ID for connection handling
    std::string buffer_;       // Buffer to accumulate request data

    FrontendServer& server;     // Frontend server object

    const std::string kHeaderEnd = "\r\n\r\n";
    const size_t kHeaderEndLen = kHeaderEnd.size();

public:
    // Constructor
    HttpConnection(int fd, FrontendServer& server);
    
    void close_connection();  // Close file descriptor and handle connection closing
    int get_fd() const;       // Retrieve the file descriptor
    void process_request();   // Process the request and send the response
    bool read_end_of_header(); // Check if end of header is reached
    HttpRequest read_request(); // Read and parse the HTTP request
    void write_response(std::string response); // Write the response to the client
    void set_thread_id(pthread_t id); // Set the thread ID

    bool validate_session(HttpRequest& request, Session& session); // Handle session for the connection
};

#endif // HTTP_CONNECTION_H
