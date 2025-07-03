#include "LoadBalancer.h"
#include "HttpConnection.h" // Full definition needed here
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

// Constructor - Initialize the load balancer
LoadBalancer::LoadBalancer(char* argv[], std::unordered_map<std::string, std::time_t>& heartbeat)  : server_heartbeat(heartbeat) {
    string config_file = argv[1];
    int line_number = 1;

    auto ip_port = get_ip_port(config_file, line_number);
    ip_addr = ip_port.first;
    port = ip_port.second;

    cout << "IP Address: " << ip_addr << endl;
    cout << "Port: " << port << endl;

    sock_fd = setup_socket(ip_addr, port);
}
// Get the next server to send the request to
string LoadBalancer::get_next_server() {
    int count = 0;
    while (true) {
        count ++;
        last_server = (last_server + 1) % server_map.size();
        auto server = server_map[last_server];
        auto it = server_heartbeat.find(server);
        if (it != server_heartbeat.end() && std::difftime(time(nullptr), it->second) < HEARTBEAT_TIMEOUT) {
            return server;
        }
        if (count == 10){
            std::cerr << "No servers available" << std::endl;
            break;
        }
    }
    return "";
}
// Read a line from the config file
string LoadBalancer::read_config_line(string filename, int line_number) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Unable to open the config file: %s\n", filename.c_str());
        exit(1);
    }
    // Read the file line by line
    const int buffer_size = 1024;
    char buffer[buffer_size];
    string line_data;
    int bytes_read, current_line = 0;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                current_line++;
                if (current_line == line_number) {
                    close(fd);
                    return line_data;
                }
                line_data.clear();
            } else {
                line_data += buffer[i];
            }
        }
    }
    // Line number not found
    fprintf(stderr, "Error: Line number %d not found in config file.\n", line_number);
    close(fd);
    exit(1);
}
// Parse the IP address and port from the string
pair<string, int> LoadBalancer::parse_ip_port(const string& ip_port) {
    size_t colon_pos = ip_port.find(':');
    if (colon_pos == string::npos) {
        fprintf(stderr, "Expected <ip:port> in the command\n");
        exit(1);
    }

    string ip = ip_port.substr(0, colon_pos);
    int port = stoi(ip_port.substr(colon_pos + 1));
    return make_pair(ip, port);
}
// Get the IP address and port from the config file at the given line number
pair<string, int> LoadBalancer::get_ip_port(string filename, int line_number) {
    string config_line = read_config_line(filename, line_number);
    return parse_ip_port(config_line);
}
// Setup the socket for the load balancer
int LoadBalancer::setup_socket(std::string ip_address, int port) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address.c_str(), &(servaddr.sin_addr));
    servaddr.sin_port = htons(port);
    ::bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(sock, QUEUE_LENGTH_MAX);
    return sock;
}
// Accept a new connection from browser
int LoadBalancer::accept_connection(int listen_fd) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    return accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
}
// Thread function to establish connection with the client and process the request
void *thread_func(void *args) {
    HttpConnection* conn = static_cast<HttpConnection*>(args);
    conn->set_thread_id(pthread_self());
    conn->process_request();
    conn->close_connection();
    delete conn;
    return nullptr;
}
// Trigger a new thread for the connection
void LoadBalancer::trigger_thread(HttpConnection* conn) {
    pthread_t child;
    pthread_create(&child, nullptr, thread_func, conn);
    pthread_detach(child);
}
// Handle the client request
void LoadBalancer::handle_client() {
    while (true) {
        int comm_fd = accept_connection(sock_fd);
        if (comm_fd < 0) {
            fprintf(stderr, "Failed to accept connection (%s)\n", strerror(errno));
            continue;
        }
        std::cout << "New connection accepted\n";
        std::cout << "File descriptor: " << comm_fd << std::endl;
        HttpConnection* connection = new HttpConnection(comm_fd, *this);
        add_connection(connection);
        trigger_thread(connection);
    }
}
// Add a new connection to the active connections
void LoadBalancer::add_connection(HttpConnection* conn) {
    int fd = conn->get_fd();
    pthread_mutex_lock(&connection_mutex);
    active_connections[fd] = conn;
    pthread_mutex_unlock(&connection_mutex);
}
// Remove a connection from the active connections
void LoadBalancer::remove_connection(int fd) {
    pthread_mutex_lock(&connection_mutex);
    auto it = active_connections.find(fd);
    if (it != active_connections.end()) {
        delete it->second;
        active_connections.erase(it);
    }
    pthread_mutex_unlock(&connection_mutex);
}
