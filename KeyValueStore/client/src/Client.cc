#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "absl/log/log.h"

#define SERVER_ADDRESS "localhost:50052"

Client::Client()
    : stub_(Server::NewStub(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()))) {}

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

int main()
{
    Client client;
    std::string value;
    client.Put("austinkt#account", "password", "123");
    client.Put("charles#account", "password", "123");
    client.Put("carlos#account", "password", "123");
    client.Put("daniel#account", "password", "123");
    client.Put("elizabeth#account", "password", "123");

    // client.Put("paul#account", "password", "123");
    // client.Put("mikhael#account", "password", "123");
    // client.Put("michelle#account", "password", "123");
    // client.Put("austin#account", "password", "123");
    // client.Put("shriya#account", "password", "123");
    // client.Put("mahika#account", "password", "123");
    // client.Put("linh#account", "password", "123");
    // client.Put("rupkatha#account", "password", "123");
    // client.Put("mikhael#account", "password", "123");
    // client.Put("michelle#account", "password", "123");
    // client.Get("austinkt#account", "password", value);
    // client.Get("austinkt#files", "jxXCR3rVfomxBGDBeFWrlZMvyeNDlnNh#data#1", value);

    // Example Get operations
    // client.Get("austin#account", "password", value);
    // client.Get("mahika#account", "password", value);
    // client.Get("mike#account", "password", value);
    // client.Get("shriya#account", "password", value);
    // client.Get("linh#account", "password", value);

    // client.Get("austin#account", "name", value);
    // client.Get("mahika#account", "name", value);
    // client.Get("mike#account", "name", value);
    // client.Get("shriya#account", "name", value);
    // client.Get("linh#account", "name", value);

    // std::unordered_map<int, std::set<std::string>> Maps::keyMappings; // cluster number -> keys A-G
    // std::unordered_map<std::string, int> Maps::serverClusterMap;      // server -> cluster Number
    // std::unordered_map<int, std::string> Maps::primaryServers;        // cluster number -> server

    // a-g
    // 1
    //

    return 0;
}
