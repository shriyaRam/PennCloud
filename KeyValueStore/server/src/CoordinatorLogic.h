#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
#include <absl/strings/str_format.h>
#include <absl/log/log.h>
#include "SharedContext.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <memory>

using namespace std;

class CoordinatorLogic final : public Coordinator::Service
{
public:
    explicit CoordinatorLogic(shared_ptr<SharedContext> sharedContext)
        : sharedContext_(move(sharedContext)) {}
    grpc::Status getStatus(grpc::ServerContext *context, const StatusArgs *args, StatusReply *reply) override;
    // grpc::Status requestServerMapping(grpc::ServerContext *context, const ServerMappingArgs *args, ServerMappingReply *reply) override;
    grpc::Status setPrimaryServer(grpc::ServerContext *context, const SetPrimaryServerArgs *args, SetPrimaryServerReply *reply) override;
    grpc::Status serverDown(grpc::ServerContext *context, const ServerDownArgs *args, ServerDownReply *reply) override;
    grpc::Status serverUp(grpc::ServerContext *context, const ServerUpArgs *args, ServerUpReply *reply) override;

private:
    shared_ptr<SharedContext> sharedContext_;
};