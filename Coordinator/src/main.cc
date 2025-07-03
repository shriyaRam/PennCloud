#include "Commons.h"
#include "Communicator.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <atomic>
#include <thread>
#define DEFAULT_ADDRESS "127.0.0.1:50050"
#define CLUSTER_SIZE 3
// function to read the config file
std::vector<std::string> readServerConfig(const std::string &configFile)
{
    std::ifstream file(configFile);
    std::vector<std::string> servers;
    // Check if the file opened successfully
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open the config file: " << configFile << std::endl;
        // Return an empty vector
        return servers;
    }

    std::cout << "Reading configuration file..." << std::endl;

    std::string line;
    while (std::getline(file, line))
    {
        servers.push_back(line); // Add each line to the vector
    }

    std::cout << "Finished reading configuration file." << std::endl;

    return servers;
}
// function to motinor the shutdown signal
void *shutdown_monitor(void *arg)
{
    // Cast the argument back to the correct type
    auto *server_data = static_cast<std::pair<std::shared_ptr<std::atomic<bool>>, grpc::Server *> *>(arg);
    auto shutdown_flag = server_data->first; // Shared pointer to the shutdown flag
    auto *grpc_server = server_data->second; // Pointer to the gRPC server

    // Monitor the shutdown flag
    while (!shutdown_flag->load())
    {
        sleep(1);
    }

    // Shutdown signal detected
    std::cout << "Shutdown signal detected. Stopping server..." << std::endl;
    grpc_server->Shutdown();

    return nullptr;
}

// thread to execute the heartbeat checker
void *heartbeat_checker_thread(void *arg)
{
    auto *data = static_cast<std::tuple<Commons *, Client *, std::vector<std::string> *> *>(arg);
    Commons *commons = std::get<0>(*data);
    Client *client = std::get<1>(*data);
    std::vector<std::string> *serverAddresses = std::get<2>(*data);

    commons->heartbeatChecker(*client, *serverAddresses);

    return nullptr;
}
// start server that listens to grpc calls
void startServer(const std::string &address)
{
    Communicator communicator;
    grpc::ServerBuilder builder;
    auto shutdown_flag = std::make_shared<std::atomic<bool>>(false);
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&communicator);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    pthread_t shutdown_thread;
    auto *server_data = new std::pair<std::shared_ptr<std::atomic<bool>>, grpc::Server *>(shutdown_flag, server.get());
    if (pthread_create(&shutdown_thread, nullptr, shutdown_monitor, server_data) != 0)
    {
        std::cerr << "Failed to create shutdown monitor thread." << std::endl;
        delete server_data;
        return;
    }

    // Shutdown signal detected
    std::cout << "Server listening on " << address << std::endl;

    server->Wait();
    pthread_join(shutdown_thread, nullptr);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string configFile = argv[1];

    // Read server configurations
    Commons::readServerConfig(configFile);
    // get all the server addresses
    std::vector<std::string> serverAddresses = Commons::getServers();

    if (serverAddresses.empty())
    {
        std::cerr << "No server configurations found!" << std::endl;
        return 1;
    }
    // Assign servers to clusters
    Maps::assignClusters(serverAddresses, CLUSTER_SIZE);

    // Create key mappings for clusters
    Maps::createKeyMappings(CLUSTER_SIZE);

    // Get and print server cluster map
    auto serverClusterMap = Maps::getServerClusterMap();

    // Get and print key mappings
    auto keyMappings = Maps::getKeyMappings();

    // Create the Client
    Client client(serverAddresses);
    // sleep for 60 seconds until all servers that should be up are up
    // sleep(30);
    // Assign primary servers
    // Commons::firstHeartbeat(client, serverAddresses);
    // Assign Primaries
    Maps::assignPrimary();

    Commons commons;

    // sleep(60);
    pthread_t heartbeat_thread;
    auto *heartbeat_data = new std::tuple<Commons *, Client *, std::vector<std::string> *>(&commons, &client, &serverAddresses);
    if (pthread_create(&heartbeat_thread, nullptr, heartbeat_checker_thread, heartbeat_data) != 0)
    {
        std::cerr << "Failed to create heartbeat checker thread." << std::endl;
        delete heartbeat_data;
        return 1;
    }
    startServer(DEFAULT_ADDRESS);
    pthread_join(heartbeat_thread, nullptr);
    delete heartbeat_data;
    return 0;
}