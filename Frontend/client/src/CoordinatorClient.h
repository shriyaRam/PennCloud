#ifndef COORDINATOR_CLIENT_H
#define COORDINATOR_CLIENT_H

#include <grpcpp/grpcpp.h>
#include "proto/coordinator.grpc.pb.h"
#include <string>
#include <map>

// CoordinatorClient is used to interact with the Coordinator gRPC service.
class CoordinatorClient {
 public:
  explicit CoordinatorClient();

  // Fetch the KVS address from the Coordinator
  std::string getKVSAddress(const std::string& username);

  // Fetch the status of all servers from the Coordinator
  std::map<std::string, bool> getAllServerStatus();

 private:
  // Stub for the Coordinator service
  std::unique_ptr<Coordinator::Stub> stub_;
};

#endif // COORDINATOR_CLIENT_H
