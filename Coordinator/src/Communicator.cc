#include "Communicator.h"
#include "Commons.h"
#include "Client.h"
#include <grpcpp/client_context.h>
#include <absl/log/log.h>
#include <tuple>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <grpcpp/create_channel.h>
#include <grpcpp/support/status.h>
#include "absl/log/log.h"
#include <unordered_map>
#include <string>
#include "proto/coordinator.pb.h"

std::unordered_map<int, int> Communicator::currentServerIndex;

// Notify the primary server of the cluster when a server is down
void Communicator::notifyPrimaryOnServerDown(Client &client, std::string &serverIpPort)
{
    const auto &serverClusterMap = Maps::getServerClusterMap();
    const auto &primaryServers = Maps::getPrimaryServers();

    if (serverClusterMap.find(serverIpPort) == serverClusterMap.end())
    {
        ABSL_LOG(WARNING) << "[NOTIFY] Server " << serverIpPort << " not found in serverClusterMap.";
        return;
    }

    // Find the cluster number for the server
    int clusterNumber = serverClusterMap.at(serverIpPort);

    // Find the primary server for this cluster
    if (primaryServers.find(clusterNumber) == primaryServers.end())
    {
        ABSL_LOG(WARNING) << "[NOTIFY] No primary server found for cluster " << clusterNumber;
        return;
    }

    std::string primaryServer = primaryServers.at(clusterNumber);

    // Create a stub and make the serverDown gRPC call
    auto stub = client.createStub(primaryServer);
    grpc::ClientContext context;
    ServerDownArgs args;
    ServerDownReply reply;

    args.set_ipaddressport(serverIpPort);

    grpc::Status status = stub->serverDown(&context, args, &reply);

    if (status.ok())
    {
        ABSL_LOG(INFO) << "[SERVERDOWN] Informed primary server " << primaryServer << " about down server " << serverIpPort << " KVS Status: " << status.error_message();
        // remove the failed server from the failedServerNotifications
        Commons::setFailedServerNotifications(serverIpPort, false);
        ABSL_LOG(INFO) << "Primary Server" << primaryServer << " knows about: " << serverIpPort;
    }
    else
    {
        ABSL_LOG(ERROR) << "[SERVERDOWN] Failed to inform primary server " << primaryServer << " about: " << serverIpPort << " Error: " << status.error_message();
        // add the failed server to the failedServerNotifications
        Commons::setFailedServerNotifications(serverIpPort, true);
    }
}

// Notify the primary server of the cluster when a server is up
void Communicator::notifyPrimaryOnServerUp(Client &client, std::string &serverIpPort)
{
    const auto &serverClusterMap = Maps::getServerClusterMap();
    const auto &primaryServers = Maps::getPrimaryServers();

    if (serverClusterMap.find(serverIpPort) == serverClusterMap.end())
    {
        ABSL_LOG(WARNING) << "[NOTIFY] Server " << serverIpPort << " not found in serverClusterMap.";
        return;
    }

    // Find the cluster number for the server
    int clusterNumber = serverClusterMap.at(serverIpPort);

    // Find the primary server for this cluster
    if (primaryServers.find(clusterNumber) == primaryServers.end())
    {
        ABSL_LOG(WARNING) << "[NOTIFY] No primary server found for cluster " << clusterNumber;
        return;
    }

    std::string primaryServer = primaryServers.at(clusterNumber);

    // Create a stub and make the serverDown gRPC call
    auto stub = client.createStub(primaryServer);
    grpc::ClientContext context;
    ServerUpArgs args;
    ServerUpReply reply;

    args.set_ipaddressport(serverIpPort);

    grpc::Status status = stub->serverUp(&context, args, &reply);

    if (status.ok())
    {
        ABSL_LOG(INFO) << "[NOTIFY: SERVER UP] Informed primary server " << primaryServer << " about server coming back up " << serverIpPort;
    }
    else
    {
        ABSL_LOG(ERROR) << "[NOTIFY: SERVER UP] Failed to inform primary server " << primaryServer << ": " << status.error_message();
    }
}

// Handle the requestServerMapping gRPC call
grpc::Status Communicator::requestServerMapping(grpc::ServerContext *context, const ServerMappingArgs *args, ServerMappingReply *reply)
{
    // Extract the server address from the request
    std::string serverAddress = args->ipaddressport();

    ABSL_LOG(INFO) << "[SERVERMAPPING] Received server mapping request for: " << serverAddress;

    // Access shared mappings for server clusters, alphabet key mappings, and primary servers
    const auto &serverClusterMap = Maps::getServerClusterMap();
    const auto &keyMappings = Maps::getKeyMappings();
    const auto &primaryServers = Maps::getPrimaryServers();

    // Check if the server is known
    if (serverClusterMap.find(serverAddress) == serverClusterMap.end())
    {
        ABSL_LOG(WARNING) << "[SERVERMAPPING] Server " << serverAddress << " not found in serverClusterMap.";
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Server not found in cluster map");
    }

    // Get the cluster number for the server
    int clusterNumber = serverClusterMap.at(serverAddress);

    // Check if the cluster has a key mapping
    if (keyMappings.find(clusterNumber) == keyMappings.end())
    {
        ABSL_LOG(WARNING) << "[SERVERMAPPING] Cluster " << clusterNumber << " not found in keyMappings.";
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Cluster not found in key mappings");
    }

    // Check if the cluster has a primary server
    if (primaryServers.find(clusterNumber) == primaryServers.end())
    {
        ABSL_LOG(WARNING) << "[SERVERMAPPING] Primary server not found for cluster " << clusterNumber;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Primary server not found for cluster");
    }

    // Retrieve the character range and primary server for this cluster
    const auto &[startChar, endChar] = keyMappings.at(clusterNumber);
    std::string primaryServer = primaryServers.at(clusterNumber);

    // Populate the reply
    reply->set_startchar(std::string(1, startChar));
    reply->set_endchar(std::string(1, endChar));
    reply->set_primaryserver(primaryServer);

    ABSL_LOG(INFO) << "[SERVERMAPPING] Server " << serverAddress
                   << " is assigned range [" << startChar << ", " << endChar
                   << "] with primary server " << primaryServer;

    return grpc::Status::OK;
}

// getter for currentServerIndex
int Communicator::getCurrentServerIndex(int clusterNo)
{
    return currentServerIndex[clusterNo];
}

// Setter for currentServerIndex
void Communicator::setCurrentServerIndex(int clusterNo, int index)
{
    currentServerIndex[clusterNo] = index;
}

// Get KVS Address in round robin fashion for fronend and email servers
grpc::Status Communicator::getKVSAddress(grpc::ServerContext *context, const GetKVSAddressArgs *args, GetKVSAddressReply *reply)
{
    // Get the username from the request
    ABSL_LOG(INFO) << "[GETKVSADDRESS]starting getKVSAddress" << std::endl
                   << std::flush;
    std::string username = args->username();
    if (username.empty())
    {
        ABSL_LOG(ERROR) << "[GETKVSADDRESS] Username is empty";
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Username is empty");
    }

    // Get the first character of the username
    char firstChar = username[0];
    ABSL_LOG(INFO) << "[GETKVSADDRESS] First character of username: " << firstChar;
    // Find the cluster for the given character
    int clusterNo = -1;
    for (const auto &entry : Maps::getKeyMappings())
    {
        char startChar, endChar;
        std::tie(startChar, endChar) = entry.second;
        if (std::toupper(firstChar) >= std::toupper(startChar) && std::toupper(firstChar) <= std::toupper(endChar))
        {

            clusterNo = entry.first;
            ABSL_LOG(INFO) << "[GETKVSADDRESS] Cluster Number for username: " << clusterNo;
            break;
        }
    }

    if (clusterNo == -1)
    {
        ABSL_LOG(INFO) << "[GETKVSADDRESS] No cluster found for the given username";
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "No cluster found for the given username");
    }

    // Get the servers in the cluster
    const std::unordered_map<int, std::vector<std::string>> &clusters = Maps::getClusters();
    auto it = clusters.find(clusterNo);
    if (it == clusters.end())
    {
        ABSL_LOG(INFO) << "[GETKVSADDRESS] Cluster number not found in clusters map.";
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Cluster number not found in clusters map");
    }

    const std::vector<std::string> &servers = it->second;

    // Initialize the cluster server index if not already initialized
    if (currentServerIndex.find(clusterNo) == currentServerIndex.end())
    {
        currentServerIndex[clusterNo] = 0;
    }

    // Find a running server in the cluster using round-robin
    int startIndex = currentServerIndex[clusterNo];
    int serverCount = servers.size();
    for (int i = 0; i < serverCount; ++i)
    {
        int index = (startIndex + i) % serverCount;
        std::string server = servers[index];
        if (Commons::getServerStatus()[server])
        {
            reply->set_ipaddressport(server);
            currentServerIndex[clusterNo] = (index + 1) % serverCount; // Update the index for round-robin
            return grpc::Status::OK;
        }
    }

    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "No running server found in the cluster");
}

// Server Status for all Servers
grpc::Status Communicator::getAllServerStatus(grpc::ServerContext *context, const GetAllServerStatusArgs *args, GetAllServerStatusReply *reply)
{
    const auto &serverStatusMap = Commons::getServerStatus();

    for (const auto &entry : serverStatusMap)
    {
        (*reply->mutable_statusmap())[entry.first] = entry.second;
    }

    return grpc::Status::OK;
}

// call every server in the cluster to send new primary
void Communicator::setPrimaryServer(const std::string &primaryServer, Client &client)
{
    ABSL_LOG(INFO) << "[SETPRIMARY] Starting primary server reassignment notification for the new primary: " << primaryServer << std::endl;
    int cluster = Maps::getServerClusterMap()[primaryServer];
    std::vector<std::string> servers = Maps::getClusters()[cluster];
    bool infoDel = false;
    // Client client(servers);
    for (std::string server : servers)
    {
        std::unordered_map<std::string, bool> serverStatusMap = Commons::getServerStatus();
        // print contents o serverStatusMap
        bool stat = false;
        stat = serverStatusMap[server];
        if (stat)
        {
            ABSL_LOG(INFO) << "[SETPRIMARY]  " << server << " is being contacted.";
            auto stub = client.createStub(server);
            grpc::ClientContext context;
            SetPrimaryServerArgs args;
            SetPrimaryServerReply reply;
            // Set a 5-second deadline for the RPC
            // std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
            // context.set_deadline(deadline);
            infoDel = true;
            args.set_primaryserver(primaryServer);
            grpc::Status status = stub->setPrimaryServer(&context, args, &reply);
            ABSL_LOG(INFO) << "[SETPRIMARY] Attempted sending new primary address to: " << server << " and got message: " << status.error_message() << std::endl;
            if (status.ok())
            {
                ABSL_LOG(INFO) << "[SETPRIMARY] Informed server " << server << " about new primary " << primaryServer;
            }
            else
            {
                ABSL_LOG(ERROR) << "[SETPRIMARY] Failed to inform server " << server << ": " << status.error_message();
            }
        }
    }
}

// gRPC function to fetch and send all the servers in the cluster of the calling server
grpc::Status Communicator::fetchAllAddress(grpc::ServerContext *context, const AllAddressArgs *args, AllAddressReply *reply)
{
    ABSL_LOG(INFO) << "Received fetchAllAddress request for: " << args->ipaddressport();

    // Check if the provided ipAddressPort exists in the serverClusterMap
    std::unordered_map<std::string, int> clusterMap = Maps::getServerClusterMap();
    if (clusterMap.find(args->ipaddressport()) == clusterMap.end())
    {
        ABSL_LOG(WARNING) << "Server address not found in cluster map: " << args->ipaddressport();
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Server address not found in cluster map");
    }

    int clusterNo = clusterMap[args->ipaddressport()];
    std::unordered_map<int, std::vector<std::string>> clusters = Maps::getClusters();
    if (clusters.find(clusterNo) == clusters.end())
    {
        ABSL_LOG(WARNING) << "Cluster number not found in clusters: " << clusterNo;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Cluster number not found in clusters");
    }
    std::vector<std::string> cluster = clusters[clusterNo];
    // print the cluster
    for (std::string &address : cluster)
    {
        reply->add_addresses(address);
    }

    ABSL_LOG(INFO) << "Successfully processed fetchAllAddress request for: " << args->ipaddressport();
    return grpc::Status::OK;
}