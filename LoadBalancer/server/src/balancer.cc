#include "LoadBalancer.h"
#include "GrpcServer.h"
#include <pthread.h>
#include <string>

// Wrapper function for pthread
void* grpc_server_thread(void* arg) {
    GrpcServer* grpc_server = static_cast<GrpcServer*>(arg);
    grpc_server->start("127.0.0.1:50020");
    return nullptr;
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./balancer <config_file> <line_number>\n");
        return 1;
    }

    // Initialize the gRPC server
    GrpcServer grpc_server;
    // Create a pthread for the gRPC server
    pthread_t grpc_thread;
    if (pthread_create(&grpc_thread, nullptr, grpc_server_thread, &grpc_server) != 0) {
        std::cerr << "Error creating gRPC server thread" << std::endl;
        return 1;
    }

    // Start the load balancer
    LoadBalancer server(argv, grpc_server.get_heartbeats());
    std::cout << "Start load balancer..." << std::endl;
    server.handle_client();
    
    // Wait for the gRPC server thread to finish
    pthread_join(grpc_thread, nullptr);

    return 0;
}
