#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include "HttpConnection.h" // Full definition needed here
#include <string>
#include <unordered_map>
#include <pthread.h>
#include <ctime>

#define QUEUE_LENGTH_MAX 1024

class HttpConnection; // Forward declaration if not included above

class LoadBalancer {
private:
    // Constants
    const int HEARTBEAT_TIMEOUT = 5; // Heartbeat timeout in seconds
    // Variables
    std::string ip_addr; // IP address of the load balancer
    int port; // Port of the load balancer
    int sock_fd; // Socket file descriptor that listens for incoming connections
    std::unordered_map<int, HttpConnection*> active_connections;  // Active connections
    pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for active connections
    std::unordered_map<std::string, std::time_t>& server_heartbeat; // Reference to the server heartbeat map
    int last_server; // Index of the last server sent to the client
    std::unordered_map<int, std::string> server_map = {
        {0, "127.0.0.1:5015"},
        {1, "127.0.0.1:5016"},
        {2, "127.0.0.1:5017"},
        {3, "127.0.0.1:5018"}
    };

public:
    LoadBalancer(char* argv[], unordered_map<std::string, std::time_t>& server_heartbeat);
    ~LoadBalancer() = default;

    std::string get_next_server(); // Get the next server to send the request to

    void handle_client(); // Handle the client request
    void trigger_thread(HttpConnection* conn); // Trigger a new thread for the connection
    void add_connection(HttpConnection* conn); //   Add a new connection to the active connections
    void remove_connection(int fd); // Remove a connection from the active connections
    static std::pair<std::string, int> get_ip_port(std::string filename, int line_number); // Get the IP address and port from the config file at the given line number

private:
    static std::string read_config_line(std::string filename, int line_number); // Read a line from the config file
    static std::pair<std::string, int> parse_ip_port(const std::string& ip_port); // Parse the IP address and port from the config line
    int setup_socket(std::string ip_address, int port); // Setup the socket for the load balancer
    int accept_connection(int listen_fd); // Accept a new connection from browser
};

#endif // LOADBALANCER_H
