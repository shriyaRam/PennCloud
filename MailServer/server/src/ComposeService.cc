#include "ComposeService.h"
#include "email.h"
#include <iostream>

// Implementation of the ComposeEmail RPC
grpc::Status ComposeService::ComposeEmail(grpc::ServerContext *context, const smtpService::ComposeEmailRequest *request, smtpService::ComposeEmailResponse *response)
{
    std::cout << "ComposeEmail RPC called" << std::flush << std::endl;
    // Extract fields from the request
    FrontendEmailData::setSender(request->sender());
    FrontendEmailData::setRecipient(request->recipient());
    FrontendEmailData::setEmailData(request->email_content());
    FrontendEmailData::setNumChunks(request->num_chunks());

    // Log the request details
    std::cout << "Received ComposeEmail request:" << std::endl
              << "  Sender: " << FrontendEmailData::getSender() << std::endl
              << "  Recipient: " << FrontendEmailData::getRecipient() << std::endl
              << "  Email content size: " << FrontendEmailData::getEmailData().size() << " bytes" << std::endl
              << "  Number of chunks: " << FrontendEmailData::getNumChunks() << std::endl;

    FrontendEmailData::storeEmailData();

    return grpc::Status::OK;
}