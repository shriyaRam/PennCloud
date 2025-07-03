#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "Client.h"
#include "../../build/proto/coordinator.grpc.pb.h"
#include <string>

class Communicator final : public Coordinator::Service
{
private:
    static std::unordered_map<int, int> currentServerIndex;

public:
    // Notify the primary server of the cluster when a server is down
    static void notifyPrimaryOnServerDown(Client &client, std::string &serverIpPort);

    // Notify the primary server of the cluster when a server is up
    static void notifyPrimaryOnServerUp(Client &client, std::string &serverIpPort);

    // Handle the requestServerMapping gRPC call
    grpc::Status requestServerMapping(grpc::ServerContext *context, const ServerMappingArgs *args, ServerMappingReply *reply) override;

    // Getter for currentServerIndex
    static int getCurrentServerIndex(int clusterNo);

    // Setter for currentServerIndex
    static void setCurrentServerIndex(int clusterNo, int index);

    // Load Balance for frontend server requests : Round Robin
    grpc::Status getKVSAddress(grpc::ServerContext *context, const GetKVSAddressArgs *args, GetKVSAddressReply *reply) override;

    // Server Status for all Servers
    grpc::Status getAllServerStatus(grpc::ServerContext *context, const GetAllServerStatusArgs *args, GetAllServerStatusReply *reply) override;

    // gRPC to send primary server to its cluster
    static void setPrimaryServer(const std::string &primaryServer, Client &client);

    // gRPC function to send all the addresses of servers in a cluster
    grpc::Status fetchAllAddress(grpc::ServerContext *context, const AllAddressArgs *args, AllAddressReply *reply) override;
};
#endif // COMMUNICATOR_H
