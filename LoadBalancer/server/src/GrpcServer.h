#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include "proto/load_balancer.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>

// gRPC Server class
class GrpcServer : public loadbalancer::LoadBalancerService::Service {
private:
    std::unordered_map<std::string, std::time_t> heartbeats; // To store server heartbeats - time is used for expiry
    std::mutex mtx; // Mutex for thread-safe access
    std::unique_ptr<grpc::Server> grpc_server;

    const int HEARTBEAT_TIMEOUT = 3; // Heartbeat timeout in seconds

public:
    GrpcServer();

    void start(const std::string& address);
    std::unordered_map<std::string, std::time_t>& get_heartbeats() { return heartbeats; }
    grpc::Status CollectHeartbeat(grpc::ServerContext* context, 
                                   const loadbalancer::HeartbeatRequest* request,
                                   loadbalancer::HeartbeatResponse* response) override;

    grpc::Status GetStatus(grpc::ServerContext* context, 
                           const loadbalancer::GetStatusRequest* request,
                           loadbalancer::GetStatusResponse* response) override;
};

#endif // GRPC_SERVER_H
