#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "Commons.h"
#include "Communicator.h"
#include "Client.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include "absl/log/log.h"
#include "proto/coordinator.pb.h"

std::vector<std::string> servers = Commons::getServers();

// function to retry heartbeat check for a server's that were detected as down
void Commons::retryServer(Client &client, std::string &server)
{
    auto stub = client.createStub(server);

    grpc::ClientContext context;
    StatusArgs args;
    StatusReply reply;

    // Set a 5-second deadline for the retry
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);

    grpc::Status status = stub->getStatus(&context, args, &reply);

    if (status.ok())
    {
        // ABSL_LOG(INFO) << "[RETRY] Server " << server << " is now OK.";
    }
    else
    {
        // ABSL_LOG(INFO) << "[RETRY] Server " << server << " is still NOT OK.";
        Commons::setServerStatus(server, false);
        Communicator::notifyPrimaryOnServerDown(client, server);
    }
}

// heartbeat checker function to check the status of all servers in the system
void Commons::heartbeatChecker(Client &client, std::vector<std::string> &servers)
{
    while (true)
    {
        for (auto &server : servers)
        {
            auto stub = client.createStub(server);

            grpc::ClientContext context;
            StatusArgs args;
            StatusReply reply;

            // 500-millosecond deadline for the RPC
            auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(500);
            context.set_deadline(deadline);

            grpc::Status status = stub->getStatus(&context, args, &reply);

            if (status.ok())
            {
                ABSL_LOG(INFO) << "[HEARTBEAT] Server " << server << " is OK.";
                if (!Commons::getServerStatus()[server])
                {
                    Maps::addServerToCluster(server, Maps::getServerClusterMap()[server]);
                    Communicator::notifyPrimaryOnServerUp(client, server);
                }
                Commons::setServerStatus(server, true);
            }
            else
            {
                ABSL_LOG(INFO) << "[HEARTBEAT] Server " << server << " is NOT OK. Retrying...";
                if (Commons::getServerStatus()[server] || Commons::getFailedServerNotifications()[server])
                {
                    Commons::retryServer(client, server);
                }
                Maps::removeServerFromCluster(server, Maps::getServerClusterMap()[server]);
                // check if one of the servers in std::unordered_map<int, std::string> Maps::primaryServers; is a failed server
                const auto &primaryServers = Maps::getPrimaryServers();
                for (const auto &[cluster, primaryServer] : primaryServers)
                {
                    if (primaryServer == server)
                    {
                        // call Commons::reassingPrimary to reassign primary
                        ABSL_LOG(INFO) << "[REASSIGN PRIMARY] Reassigning the primary since " << server << " is NOT OK.";
                        Maps::reassignPrimary(server, client);
                        // call primaryAssignment to send IP:port of new primary server to all servers in that cluster
                    }
                }
            }
        }

        // Sleep for a short duration before the next round of checks
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// a function from Commons for only one heartbeat check for each server in a for loop without retry
bool Commons::singleHeartbeat(Client &client, std::string &server)
{

    auto stub = client.createStub(server);

    grpc::ClientContext context;
    StatusArgs args;
    StatusReply reply;

    // Set a 5-second deadline for the RPC
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);

    grpc::Status status = stub->getStatus(&context, args, &reply);

    if (status.ok())
    {
        // ABSL_LOG(INFO) << "[FIRST HEARTBEAT] Server " << server << " is OK.";
        Commons::setServerStatus(server, true);
        return true;
    }
    else
    {
        // ABSL_LOG(INFO) << "[FIRST HEARTBEAT] Server " << server << " is NOT OK.";
        Commons::setServerStatus(server, false);
        return false;
    }
}
