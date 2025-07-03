#include "GRPCCalls.h"
#include <grpcpp/grpcpp.h>
#include <iostream>

std::unique_ptr<Coordinator::Stub> GRPCCalls::stub_;

GRPCCalls::GRPCCalls()
{
    // Initialize the stub for the Coordinator service
    auto channel = grpc::CreateChannel("127.0.0.1:50050", grpc::InsecureChannelCredentials());
    stub_ = Coordinator::NewStub(channel);
}

std::string GRPCCalls::getKVSAddress(const std::string &username)
{
    // Create a gRPC client context
    std::cout << "In getKVSAddress" << std::endl
              << std::flush;
    grpc::ClientContext context;

    // Create the request and response objects
    GetKVSAddressArgs request;
    GetKVSAddressReply response;

    // Set the username in the request
    request.set_username(username);
    std::cout << "request user name set" << username << std::endl
              << std::flush;
    // Make the gRPC call
    grpc::Status status = stub_->getKVSAddress(&context, request, &response);

    std::cout << "after gRPC call" << std::endl
              << std::flush;
    if (status.ok())
    {
        return response.ipaddressport();
    }
    else
    {
        std::cerr << "Error: " << status.error_message() << std::endl;
        return "";
    }
}