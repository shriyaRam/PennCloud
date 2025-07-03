#ifndef EMAIL_CLIENT_H
#define EMAIL_CLIENT_H

#include <grpcpp/grpcpp.h>
#include "proto/smtp_server.grpc.pb.h"

class EmailClient
{
public:
  // Constructor to initialize the client with the server address
  explicit EmailClient(const std::string &server_address);

  // Method to compose an email
  bool ComposeEmail(const std::string &sender, const std::string &recipient,
                    const std::string &email_content, int32_t num_chunks);

  // Method to forward an email
  bool ForwardEmail(const std::string &sender, const std::string &recipient,
                    const std::string &email_content, int32_t num_chunks);

private:
  std::unique_ptr<smtpService::SMTPServerService::Stub> stub_; // Stub for the gRPC service
};
#endif // EMAIL_CLIENT_H