#include "EmailClient.h"
#include <iostream>
#include <unordered_map>

std::unordered_map<std::string, std::string> server_map = {
    {"127.0.0.1:5015", "localhost:5000"},
    {"127.0.0.1:5016", "localhost:5000"},
    {"127.0.0.1:5017", "localhost:5000"},
    {"127.0.0.1:5018", "localhost:5000"},
};

// Constructor
EmailClient::EmailClient(const std::string &server_address)
    : stub_(smtpService::SMTPServerService::NewStub(grpc::CreateChannel(
          server_map[server_address], grpc::InsecureChannelCredentials()))) {}

// Method to compose an email
bool EmailClient::ComposeEmail(const std::string &sender, const std::string &recipient,
                               const std::string &email_content, int32_t num_chunks)
{
    std::cout << "in compose email" << std::endl;
    // Prepare the request
    smtpService::ComposeEmailRequest request;
    request.set_sender(sender);
    request.set_recipient(recipient);
    request.set_email_content(email_content); // Assumes email_content is a string
    request.set_num_chunks(num_chunks);
    std::cout << "Set Request " << std::endl;
    smtpService::ComposeEmailResponse response;
    grpc::ClientContext context;

    std::cout << "Email from: " << sender << std::endl;
    std::cout << "Sending email to " << recipient << std::endl;
    std::cout << email_content << std::endl;
    std::cout << "Returning from compose email" << std::endl;

    // Make the RPC call
    grpc::Status status = stub_->ComposeEmail(&context, request, &response);

    // Check the result of the RPC
    if (status.ok())
    {
        std::cout << "Email composed successfully!" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to compose email: " << status.error_message() << std::endl;
        return false;
    }
    return true;
}

// Method to compose an email
// bool EmailClient::ForwardEmail(const std::string &sender, const std::string &recipient,
//                                const std::string &email_content, int32_t num_chunks)
// {
//     // Prepare the request
//     smtp::ForwardEmailRequest request;
//     request.set_sender(sender);
//     request.set_recipient(recipient);
//     request.set_email_content(email_content); // Assumes email_content is a string
//     request.set_num_chunks(num_chunks);

//     smtp::ForwardEmailResponse response;
//     grpc::ClientContext context;

//     std::cout << "Email from: " << sender << std::endl;
//     std::cout << "Forwarding email to " << recipient << std::endl;
//     std::cout << email_content << std::endl;
//     std::cout << "Returning from compose email" << std::endl;

//     // Make the RPC call
//     grpc::Status status = stub_->ComposeEmail(&context, request, &response);

//     // Check the result of the RPC
//     if (status.ok())
//     {
//         std::cout << "Email composed successfully!" << std::endl;
//         return true;
//     }
//     else
//     {
//         std::cerr << "Failed to compose email: " << status.error_message() << std::endl;
//         return false;
//     }
//     return true;
// }