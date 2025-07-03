#ifndef CLIENT_H_
#define CLIENT_H_
#include <grpcpp/grpcpp.h>
#include "proto/coordinator.pb.h"
#include "proto/coordinator.grpc.pb.h"

class Client
{
public:
    // Constructor: Initializes the server list
    explicit Client(const std::vector<std::string> &servers);

    // Creates and returns a new stub for the specified server
    std::unique_ptr<Coordinator::Stub> createStub(const std::string &server) const;
};

#endif // CLIENT_H_
