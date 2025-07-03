#ifndef SERVER_H_
#define SERVER_H_

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "proto/server.pb.h"
#include "proto/server.grpc.pb.h"
#include <unordered_map>
#include <absl/strings/str_format.h>
#include <absl/log/log.h>
#include "SharedContext.h"

using namespace std;

class ServiceLogic final : public Server::Service
{
public:
    explicit ServiceLogic(std::shared_ptr<SharedContext> sharedContext, std::shared_ptr<std::atomic<bool>> shutdown_flag)
        : sharedContext_(std::move(sharedContext)), shutdown_flag(std::move(shutdown_flag)) {}
    grpc::Status Put(grpc::ServerContext *context, const PutArgs *args, PutReply *reply) override;
    grpc::Status Get(grpc::ServerContext *context, const GetArgs *args, GetReply *reply) override;
    grpc::Status CPut(grpc::ServerContext *context, const CPutArgs *args, CPutReply *reply) override;
    grpc::Status Del(grpc::ServerContext *context, const DelArgs *args, DelReply *reply) override;
    grpc::Status Kill(grpc::ServerContext *context, const KillArgs *args, KillReply *reply) override;
    grpc::Status FetchKeys(grpc::ServerContext *context, const KeyArgs *args, KeyReply *reply) override;
    grpc::Status Revive(grpc::ServerContext *context, const ReviveArgs *args, ReviveReply *reply) override;

private:
    shared_ptr<SharedContext> sharedContext_;
    std::shared_ptr<std::atomic<bool>> shutdown_flag;
};

#endif // SERVER_H_
