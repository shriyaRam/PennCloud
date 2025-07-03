#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include "absl/log/log.h"

using namespace std;
Client::Client(Session &session_ref)
    : session(session_ref)
{
    // Check if the address is empty
    std::string address = session_ref.get_address_port();
    if (address.empty())
    {
        address = "localhost:50000"; // This will fail and the program should contact the coordinator
    }

    // Set up the stub with the resolved address
    stub_ = Server::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
}
// Update the corrsponding kvs address based on the username
void Client::updateAddress()
{
    session.set_address_port(coordinator_client.getKVSAddress(session.get_username()));
    stub_ = Server::NewStub(grpc::CreateChannel(session.get_address_port(), grpc::InsecureChannelCredentials()));
    std::cout << "[Updated address:] " << session.get_address_port() << std::endl;
    sleep(1);
}
// PUT method: Stores a value for a specific key and column in the BigTable
bool Client::Put(const std::string &row, const std::string &column, const std::string &value)
{

    PutArgs args;
    PutReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column
    args.set_value(value);   // Set the value

    int retry = 0;

    while (true)
    {
        grpc::ClientContext context;
        grpc::Status status = stub_->Put(&context, args, &reply);
        if (!status.ok())
        {
            ABSL_LOG(INFO) << "[PUT] Error: " << status.error_message();
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE && retry < 3 || status.error_message().find("Failed to create secure client channel") != std::string::npos) // retry 3 times
            {
                retry++;
                updateAddress();
                continue;
            }
            return false;
        }
        // std::string print_value = value.size() > 20 ? value.substr(0, 20) + "..." : value;
        std::string print_value = value;
        ABSL_LOG(INFO) << "[PUT] Row: " << row << ", Column: " << column << ", Value: " << print_value;
        return true;
    }
}
// GET method: Retrieves the value for a specific key and column from the BigTable
bool Client::Get(const std::string &row, const std::string &column, std::string &value)
{

    GetArgs args;
    GetReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column

    int retry = 0;

    while (true)
    {
        grpc::ClientContext context;
        std::cout << "Get request to: " << session.get_address_port() << std::endl;
        std::cout << "Row: " << row << " Column: " << column << std::endl;
        grpc::Status status = stub_->Get(&context, args, &reply);

        if (!status.ok())
        {
            ABSL_LOG(INFO) << "[GET] Error: " << status.error_message();

            if (status.error_code() == grpc::StatusCode::UNAVAILABLE && retry < 3) // retry 3 times
            {
                retry++;
                updateAddress();
                continue;
            }
            return false;
        }

        value = reply.value(); // Set the retrieved value
        std::string print_value = value.size() > 20 ? value.substr(0, 20) + "..." : value;
        ABSL_LOG(INFO) << "[GET] Row: " << row << ", Column: " << column << ", Value: " << print_value;
        return true;
    }
}
// CPut method: Compares and swaps the value for a specific key and column in the BigTable
bool Client::CPut(const std::string &row, const std::string &column, const std::string &value1, const std::string &value2)
{

    CPutArgs args;
    CPutReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column
    args.set_value1(value1); // Set the old value to check
    args.set_value2(value2); // Set the new value to store

    int retry = 0;

    while (true)
    {
        grpc::ClientContext context;
        grpc::Status status = stub_->CPut(&context, args, &reply);

        if (!status.ok())
        {
            ABSL_LOG(INFO) << "[CPUT] Error: " << status.error_message();
            // If the server is down, retry 3 times to find another one
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE && retry < 3) // retry 3 times
            {
                retry++;
                updateAddress();
                continue;
            }
            return false;
        }

        ABSL_LOG(INFO) << "[CPUT] Row: " << row << ", Column: " << column << ", Value1: " << value1 << ", Value2: " << value2;
        return true;
    }
}
// Del method: Deletes the value for a specific key and column in the BigTable
bool Client::Del(const std::string &row, const std::string &column)
{

    DelArgs args;
    DelReply reply;

    args.set_row(row);       // Set the row
    args.set_column(column); // Set the column

    int retry = 0;

    while (true)
    {
        grpc::ClientContext context;
        grpc::Status status = stub_->Del(&context, args, &reply);
        if (!status.ok())
        {
            ABSL_LOG(INFO) << "[DEL] Error: " << status.error_message();
            // If the server is down, retry 3 times to find another one
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE && retry < 3) // retry 3 times
            {
                retry++;
                updateAddress();
                continue;
            }
            return false;
        }

        ABSL_LOG(INFO) << "[DELETE] Row: " << row << ", Column: " << column;
        return true;
    }
}
// Kill method: Kills the server
bool Client::Kill()
{
    grpc::ClientContext context;
    KillArgs args;   // Empty
    KillReply reply; // Empty

    // Call the Kill RPC
    grpc::Status status = stub_->Kill(&context, args, &reply);
    std::cout << "Killing server: " << std::endl;
    std::cout << session.get_address_port() << std::endl;
    // if (!status.ok())
    // {
    //     ABSL_LOG(INFO) << "[KILL] Error: " << status.error_message();
    //     return false;
    // }
    cout << status.error_message() << endl;
    ABSL_LOG(INFO)
        << "[KILL] Successfully sent Kill request to server";
    return true;
}
// FetchKeys method: Fetches all the keys and columns from the BigTable
bool Client::FetchKeys(std::map<std::string, std::vector<std::string>> &keys)
{
    KeyArgs args;
    KeyReply reply;
    int retry = 0;

    while (true)
    {
        grpc::ClientContext context;
        grpc::Status status = stub_->FetchKeys(&context, args, &reply);
        
        if (!status.ok())
        {
            ABSL_LOG(INFO) << "[FetchKeys] Error: " << status.error_message();
            // If the server is down, retry 3 times to find another one
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE && retry < 3)
            {
                ABSL_LOG(INFO) << "[Retry] FetchKeys failed. Retrying...";
                retry++;
                continue;
            }
            return false;
        }

        // Parse the map<string, CustomValue> from the reply
        for (const auto &pair : reply.my_map())
        {
            std::vector<std::string> value_list;
            for (const auto &val : pair.second.values())
            {
                value_list.push_back(std::string(val.begin(), val.end()));
            }
            keys[pair.first] = value_list;
        }

        ABSL_LOG(INFO) << "[FetchKeys] Successfully fetched keys.";
        return true;
    }
}
// Revive method: Revives a server that was previously killed
bool Client::Revive()
{
    grpc::ClientContext context;
    ReviveArgs args;
    ReviveReply reply;

    grpc::Status status = stub_->Revive(&context, args, &reply);

    if (!status.ok())
    {
        ABSL_LOG(INFO) << "[Revive] Error: " << status.error_message();
        return false;
    }

    ABSL_LOG(INFO) << "[Revive] Successfully sent Revive request to server";
    return true;
}
