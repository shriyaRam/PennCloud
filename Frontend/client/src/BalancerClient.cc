#include "BalancerClient.h"
#include <iostream>

// Constructor
BalancerClient::BalancerClient()
    : stub_(loadbalancer::LoadBalancerService::NewStub(
          grpc::CreateChannel("127.0.0.1:50020", grpc::InsecureChannelCredentials()))) {}

// Send a heartbeat to the Load Balancer
void BalancerClient::send_heartbeat(const std::string& server_id, const std::string& status) {
    loadbalancer::HeartbeatRequest request;
    request.set_server_id(server_id);
    request.set_status(status);

    loadbalancer::HeartbeatResponse response;
    grpc::ClientContext context;

    grpc::Status rpc_status = stub_->CollectHeartbeat(&context, request, &response);

    // if (rpc_status.ok()) {
    //     std::cout << "Heartbeat sent successfully: " << response.message() << std::endl;
    // } else {
    //     std::cerr << "Failed to send heartbeat: " << rpc_status.error_message() << std::endl;
    // }
}

// Query the Load Balancer for status
std::string BalancerClient::get_status(const std::string& key) {
    loadbalancer::GetStatusRequest request;
    request.set_key(key);

    loadbalancer::GetStatusResponse response;
    grpc::ClientContext context;

    grpc::Status rpc_status = stub_->GetStatus(&context, request, &response);

    if (rpc_status.ok()) {
        std::cout << response.value() << std::endl;
        return response.value();
    } else {
        std::cerr << "Failed to get status: " << rpc_status.error_message() << std::endl;
        return "Error";
    }
}
