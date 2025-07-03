#ifndef CLIENT_H_
#define CLIENT_H_

#include <grpcpp/grpcpp.h>
#include "proto/server.pb.h"
#include "proto/server.grpc.pb.h"
#include "CoordinatorClient.h"
#include "Session.h"

class Client
{

private:

    Session& session;
    // The gRPC stub used for communication with the Server service
    CoordinatorClient coordinator_client; // Coordinator client

public:

    // Constructor: Initializes the gRPC stub for the Server service
    Client(Session& session_ref); 

    // PUT method: Stores a value for a specific key and column in the BigTable
    bool Put(const std::string &key, const std::string &column, const std::string &value);

    // GET method: Retrieves the value for a specific key and column from the BigTable
    bool Get(const std::string &key, const std::string &column, std::string &value);
    // CPut method: Compares and swaps the value for a specific key and column in the BigTable
    bool CPut(const std::string &row, const std::string &column, const std::string &value1, const std::string &value2);
    // Del method: Deletes the value for a specific key and column in the BigTable
    bool Del(const std::string &row, const std::string &column);
    // Kill method: Kills the server
    bool Kill();

    // FetchKeys method: Fetches all keys from the BigTable
    bool FetchKeys(std::map<std::string, std::vector<std::string>>& keys);
    // Revive method: Revives the server
    bool Revive();

    // Update the address of the kvs server based on the username in the session
    void updateAddress();

private:
    // The gRPC stub used for communication with the Server service
    std::unique_ptr<Server::Stub> stub_;
};

#endif // CLIENT_H_
