#ifndef GRPCCALLS_H
#define GRPCCALLS_H

#include <grpcpp/grpcpp.h>
#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
#include <string>
#include <map>

// CoordinatorClient is used to interact with the Coordinator gRPC service.
class GRPCCalls
{
public:
    explicit GRPCCalls();

    // Fetch the KVS address from the Coordinator
    static std::string getKVSAddress(const std::string &username);

private:
    // Stub for the Coordinator service
    static std::unique_ptr<Coordinator::Stub> stub_;
};

#endif // GRPCCALLS_H