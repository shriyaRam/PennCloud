#ifndef CLIENT_H_
#define CLIENT_H_

#include <grpcpp/grpcpp.h>
#include "proto/server.pb.h"
#include "proto/server.grpc.pb.h"

class Client
{
public:
    // Constructor: Initializes the gRPC stub for the Server service
    Client();

    // PUT method: Stores a value for a specific key and column in the BigTable
    bool Put(const std::string &key, const std::string &column, const std::string &value);

    // GET method: Retrieves the value for a specific key and column from the BigTable
    bool Get(const std::string &key, const std::string &column, std::string &value);

    bool CPut(const std::string &row, const std::string &column, const std::string &value1, const std::string &value2);

    bool Del(const std::string &row, const std::string &column);

private:
    // The gRPC stub used for communication with the Server service
    std::unique_ptr<Server::Stub> stub_;
};

#endif // CLIENT_H_
