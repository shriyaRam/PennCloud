#ifndef FRONTEND_SERVER_H
#define FRONTEND_SERVER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <pthread.h>
#include "Session.h"

// Forward declaration of HttpConnection
class HttpConnection;

class FrontendServer {
private:
    // Server configuration
    std::string ip_addr;
    int port;
    int sock_fd;

    // Active connections
    std::unordered_map<int, HttpConnection*> active_connections;
    pthread_mutex_t connection_mutex;

    // Session management
    std::unordered_map<std::string, Session> sessions;
    pthread_mutex_t session_mutex;

    const int QUEUE_LENGTH_MAX = 100;

    // Helper methods
    std::string read_config_line(std::string filename, int line_number);
    std::pair<std::string, int> parse_ip_port(const std::string& ip_port);
    std::pair<std::string, int> get_ip_port(std::string filename, int line_number);
    int setup_socket(std::string ip_address, int port);
    int accept_connection(int listen_fd);

public:
    // Constructor
    FrontendServer(char* argv[]);

    // Session management methods
    bool is_session_valid(const std::string& session_id);
    Session get_session(const std::string& session_id);
    void create_session(const std::string& session_id, const Session& session_data);
    void remove_session(const std::string& session_id);
    std::string get_server_id();

    // Connection management
    void handle_client();
    void add_connection(HttpConnection* conn);
    void remove_connection(int fd);
    void trigger_thread(HttpConnection* conn);
};

#endif // FRONTEND_SERVER_H
