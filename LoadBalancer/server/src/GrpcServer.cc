#include "GrpcServer.h"
#include <iostream>

// Constructor
GrpcServer::GrpcServer() {
    // Initialize the map
    heartbeats = {
        {"127.0.0.1:5015", 0},
        {"127.0.0.1:5016", 0},
        {"127.0.0.1:5017", 0},
        {"127.0.0.1:5018", 0},
    };
}

// Start the gRPC server
void GrpcServer::start(const std::string& address) {
    grpc::ServerBuilder builder;

    // Add listening port
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());

    // Register the service
    builder.RegisterService(this);

    // Build and start the server
    grpc_server = builder.BuildAndStart();
    std::cout << "gRPC server is listening on " << address << std::endl;

    // Wait for the server to shutdown
    grpc_server->Wait();
}

// CollectHeartbeat RPC implementation
grpc::Status GrpcServer::CollectHeartbeat(grpc::ServerContext* context, 
                                          const loadbalancer::HeartbeatRequest* request,
                                          loadbalancer::HeartbeatResponse* response) {
    std::lock_guard<std::mutex> lock(mtx);

    // Store the heartbeat information
    time_t now = time(nullptr);
    heartbeats[request->server_id()] = now;

    std::cout << "Received heartbeat from " << request->server_id() << std::endl;
    // Send acknowledgment response
    response->set_message("Heartbeat acknowledged");
    return grpc::Status::OK;
}

// GetStatus RPC implementation
grpc::Status GrpcServer::GetStatus(grpc::ServerContext* context, 
                                   const loadbalancer::GetStatusRequest* request,
                                   loadbalancer::GetStatusResponse* response) {
    std::lock_guard<std::mutex> lock(mtx);

    std::string status;
    for (const auto& entry : heartbeats) {
        if (std::difftime(time(nullptr), entry.second) > HEARTBEAT_TIMEOUT) {
            status += entry.first + "#" + "Down" + "#";
        } else {
            status += entry.first + "#" + "Running" + "#";
        }
    }
    response->set_value(status);
    return grpc::Status::OK;
}
