#include "CoordinatorClient.h"
#include <iostream>

// Constructor
CoordinatorClient::CoordinatorClient()
    : stub_(Coordinator::NewStub(grpc::CreateChannel("127.0.0.1:50050", grpc::InsecureChannelCredentials()))) {}

// Fetch the KVS address from the Coordinator
std::string CoordinatorClient::getKVSAddress(const std::string& username) {
  GetKVSAddressArgs request;
  GetKVSAddressReply reply;
  grpc::ClientContext context;
  // Set the username in the request
  request.set_username(username);
  std::cout << "Requesting KVS address for " << username << std::endl;
  grpc::Status status = stub_->getKVSAddress(&context, request, &reply);

  if (!status.ok()) {
    std::cerr << "Error calling getKVSAddress: " << status.error_message() << std::endl;
    return "";
  }

  return reply.ipaddressport();
}

// Fetch the status of all kvs servers from the Coordinator
std::map<std::string, bool> CoordinatorClient::getAllServerStatus() {
  GetAllServerStatusArgs request;
  GetAllServerStatusReply reply;
  grpc::ClientContext context;

  grpc::Status status = stub_->getAllServerStatus(&context, request, &reply);

  if (!status.ok()) {
    std::cerr << "Error calling getAllServerStatus: " << status.error_message() << std::endl;
    return {};
  }

  // Convert the protobuf map to std::map and return
  std::map<std::string, bool> status_map;
  for (const auto& entry : reply.statusmap()) {
    status_map[entry.first] = entry.second;
  }

  return status_map;
}
