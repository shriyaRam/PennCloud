#ifndef SMTP_SERVER_H
#define SMTP_SERVER_H

#include <grpcpp/grpcpp.h>
#include "proto/smtp_server.grpc.pb.h"

// Implements the methods defined in the gRPC service
class ComposeService final : public smtpService::SMTPServerService::Service {
public:
    ::grpc::Status ComposeEmail(::grpc::ServerContext* context,
                                const smtpService::ComposeEmailRequest* request,
                                smtpService::ComposeEmailResponse* response) override;
};

#endif // SMTP_SERVER_H
