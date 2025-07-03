#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "absl/log/log.h"
#include <stdio.h>

Client::Client(const std::string &server_address)
    : stub_(Server::NewStub(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))) {}

bool Client::Put(const std::string &row, const std::string &column, const std::string &value)
{
    grpc::ClientContext context;
    PutArgs args;
    PutReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column
    args.set_value(value);   // Set the value

    grpc::Status status = stub_->Put(&context, args, &reply);

    if (!status.ok())
    {
        std::cout << "row " << row << std::endl;
        std::cout << "col " << column << std::endl;
        std::cout << "value " << value << std::endl;

        ABSL_LOG(INFO) << "[PUT] Error: " << status.error_message();
        return false;
    }

    ABSL_LOG(INFO) << "[PUT] Row: " << row << ", Column: " << column << ", Value: " << value;
    return true;
}

bool Client::Get(const std::string &row, const std::string &column, std::string &value)
{
    grpc::ClientContext context;
    GetArgs args;
    GetReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column

    grpc::Status status = stub_->Get(&context, args, &reply);

    if (!status.ok())
    {
        ABSL_LOG(INFO) << "[GET] Error: " << status.error_message();
        return false;
    }

    value = reply.value(); // Set the retrieved value
    ABSL_LOG(INFO) << "[GET] Row: " << row << ", Column: " << column << ", Value: " << value;
    return true;
}

bool Client::CPut(const std::string &row, const std::string &column, const std::string &value1, const std::string &value2)
{
    grpc::ClientContext context;
    CPutArgs args;
    CPutReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column
    args.set_value1(value1); // Set the old value to check
    args.set_value2(value2); // Set the new value to store

    grpc::Status status = stub_->CPut(&context, args, &reply);

    if (!status.ok())
    {
        ABSL_LOG(INFO) << "[CPUT] Error: " << status.error_message();
        return false;
    }

    ABSL_LOG(INFO) << "[CPUT] Row: " << row << ", Column: " << column << ", Value1: " << value1 << ", Value2: " << value2;
    return true;
}

bool Client::Del(const std::string &row, const std::string &column)
{
    grpc::ClientContext context;
    DelArgs args;
    DelReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column

    grpc::Status status = stub_->Del(&context, args, &reply);

    if (!status.ok())
    {
        ABSL_LOG(INFO) << "[DELETE] Error: " << status.error_message();
        return false;
    }

    ABSL_LOG(INFO) << "[DELETE] Row: " << row << ", Column: " << column;
    return true;
}