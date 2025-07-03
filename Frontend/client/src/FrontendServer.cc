#include "FrontendServer.h"
#include "HttpConnection.h"  // Full definition needed here
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

FrontendServer::FrontendServer(char* argv[]): connection_mutex(PTHREAD_MUTEX_INITIALIZER), session_mutex(PTHREAD_MUTEX_INITIALIZER) {
    string config_file = argv[1];
    int line_number = stoi(argv[2]);

    auto ip_port = get_ip_port(config_file, line_number);
    ip_addr = ip_port.first;
    port = ip_port.second;

    cout << "IP Address: " << ip_addr << endl;
    cout << "Port: " << port << endl;

    sock_fd = setup_socket(ip_addr, port);
}

std::string FrontendServer::get_server_id(){
    return ip_addr + ":" + std::to_string(port);
}

bool FrontendServer::is_session_valid(const std::string& session_id) {
    pthread_mutex_lock(&session_mutex);
    auto it = sessions.find(session_id);
    pthread_mutex_unlock(&session_mutex);
    return it != sessions.end() && !it->second.is_expired();
}

Session FrontendServer::get_session(const std::string& session_id){
    pthread_mutex_lock(&session_mutex);
    auto it = sessions.find(session_id);
    pthread_mutex_unlock(&session_mutex);
    return it->second;
}

void FrontendServer::create_session(const std::string& session_id, const Session& session_data) {
    pthread_mutex_lock(&session_mutex);
    sessions[session_id] = session_data;
    pthread_mutex_unlock(&session_mutex);
}

void FrontendServer::remove_session(const std::string& session_id) {
    pthread_mutex_lock(&session_mutex);
    sessions.erase(session_id);
    pthread_mutex_unlock(&session_mutex);
}

string FrontendServer::read_config_line(string filename, int line_number) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Unable to open the config file: %s\n", filename.c_str());
        exit(1);
    }

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

    fprintf(stderr, "Error: Line number %d not found in config file.\n", line_number);
    close(fd);
    exit(1);
}

pair<string, int> FrontendServer::parse_ip_port(const string& ip_port) {
    size_t colon_pos = ip_port.find(':');
    if (colon_pos == string::npos) {
        fprintf(stderr, "Expected <ip:port> in the command\n");
        exit(1);
    }

    string ip = ip_port.substr(0, colon_pos);
    int port = stoi(ip_port.substr(colon_pos + 1));
    return make_pair(ip, port);
}

pair<string, int> FrontendServer::get_ip_port(string filename, int line_number) {
    string config_line = read_config_line(filename, line_number);
    return parse_ip_port(config_line);
}

int FrontendServer::setup_socket(std::string ip_address, int port) {
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

int FrontendServer::accept_connection(int listen_fd) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    return accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
}

void *thread_func(void *args) {
    HttpConnection* conn = static_cast<HttpConnection*>(args);
    conn->set_thread_id(pthread_self());
    conn->process_request();
    conn->close_connection();
    delete conn;
    return nullptr;
}

void FrontendServer::trigger_thread(HttpConnection* conn) {
    pthread_t child;
    pthread_create(&child, nullptr, thread_func, conn);
    pthread_detach(child);
}

void FrontendServer::handle_client() {

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

void FrontendServer::add_connection(HttpConnection* conn) {
    int fd = conn->get_fd();
    pthread_mutex_lock(&connection_mutex);
    active_connections[fd] = conn;
    pthread_mutex_unlock(&connection_mutex);
}

void FrontendServer::remove_connection(int fd) {
    pthread_mutex_lock(&connection_mutex);
    auto it = active_connections.find(fd);
    if (it != active_connections.end()) {
        delete it->second;
        active_connections.erase(it);
    }
    pthread_mutex_unlock(&connection_mutex);
}
