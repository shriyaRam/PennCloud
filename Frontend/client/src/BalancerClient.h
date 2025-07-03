#ifndef BALANCER_CLIENT_H
#define BALANCER_CLIENT_H

#include "proto/load_balancer.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <string>

class BalancerClient {
private:
    std::unique_ptr<loadbalancer::LoadBalancerService::Stub> stub_;

public:
    // Constructor
    BalancerClient();

    // Send a heartbeat to the Load Balancer (for load balancing)
    void send_heartbeat(const std::string& server_id, const std::string& status);

    // Query the Load Balancer for status (for admin purposes)
    std::string get_status(const std::string& key);
};

#endif // BALANCER_CLIENT_H
