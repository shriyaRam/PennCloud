#ifndef INTER_SERVER_LOGIC_H
#define INTER_SERVER_LOGIC_H

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "proto/interserver.grpc.pb.h"
#include "proto/interserver.pb.h"
#include <absl/strings/str_format.h>
#include <absl/log/log.h>
#include "SharedContext.h"

using namespace std;

class InterServerLogic final : public InterServer::Service
{
public:
    explicit InterServerLogic(shared_ptr<SharedContext> sharedContext)
        : sharedContext_(move(sharedContext)) {}
    grpc::Status callPrimary(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply) override;
    grpc::Status sendToServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply) override;
    grpc::Status askServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply) override;
    grpc::Status rollBackServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply) override;
    grpc::Status sendCheckPoint(grpc::ServerContext *context, const CheckPointArgs *args, CheckPointReply *reply) override;
    grpc::Status checkpointing(grpc::ServerContext *context, const CheckPointArgs *args, CheckPointReply *reply) override;
    grpc::Status loadNewTablet(grpc::ServerContext *context, const LoadTabletArgs *args, LoadTabletReply *reply) override;
    grpc::Status fetchCurrentTabletIndex(grpc::ServerContext *context, const TabletIndexArgs *args, TabletIndexReply *reply) override;
    grpc::Status getMissingLogsFromPosition(grpc::ServerContext *context, const LogPositionRequest *request, LogPositionResponse *response) override;
    grpc::Status getCheckpointChunk(grpc::ServerContext *context, const CheckpointChunkRequest *args, CheckpointChunkResponse *reply) override;
    grpc::Status getLogChunk(grpc::ServerContext *context, const CheckpointChunkRequest *args, CheckpointChunkResponse *reply) override;
    grpc::Status checkPointAndEvictForRead(grpc::ServerContext *context, const ReadEvictArgs *args, ReadEvictReply *reply) override;

private:
    shared_ptr<SharedContext> sharedContext_;
};

#endif // INTER_SERVER_LOGIC_H