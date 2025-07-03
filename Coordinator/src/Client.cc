#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "absl/log/log.h"
#include "Commons.h"
#include "../../build/proto/coordinator.grpc.pb.h"

// Class to create a client stub for making gRPC calls
Client::Client(const std::vector<std::string> &servers)
{
}
std::unique_ptr<Coordinator::Stub> Client::createStub(const std::string &server) const
{
    auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
    return Coordinator::NewStub(channel);
}