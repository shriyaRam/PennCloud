#include "Commons.h"
#include "Communicator.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <cmath>

// key: cluster number ; value: tuple with (char, char)
std::unordered_map<int, std::tuple<char, char>> Maps::keyMappings;
// key: server ip:port ; value: cluster number
std::unordered_map<std::string, int> Maps::serverClusterMap;
// key: cluster number; value: ip:port value
std::unordered_map<int, std::string> Maps::primaryServers;
// key: cluster number; value: vector of ip:port values
std::unordered_map<int, std::vector<std::string>> Maps::clusters;

void Maps::assignPrimary()
{
    // Clear existing primary server assignments
    primaryServers.clear();

    std::cout << "Assigning primary servers..." << std::endl;

    for (const auto &[server, cluster] : serverClusterMap)
    {
        // Print server and cluster
        std::cout << "Checking server: " << server << " for cluster: " << cluster << std::endl;

        // Check server status
        bool serverStatus = Commons::getServerStatus()[server];
        std::cout << "Server status for " << server << ": " << (serverStatus ? "UP" : "DOWN") << std::endl;

        // If the cluster doesn't have a primary server yet, assign this server
        if (primaryServers.find(cluster) == primaryServers.end())
        {
            std::cout << "Assigning server: " << server << " as primary for cluster: " << cluster << std::endl;
            primaryServers[cluster] = server;
        }
    }

    std::cout << "Primary server assignment complete." << std::endl;
}

// reassign primary server
void Maps::reassignPrimary(std::string oldprimary, Client &client)
{
    std::cout << "[REASSIGN PRIMARY] starts here" << std::endl;
    bool primaryReassigned = false;
    // get cluster number of old primary
    int cluster = serverClusterMap[oldprimary];
    // get all servers that are up in cluster
    std::vector<std::string> servers = clusters[cluster];
    // check for the next alive server in the cluster
    for (std::string &server : servers)
    {
        // if the server is not the old primary and is up
        if (server != oldprimary && Commons::getServerStatus()[server])
        {
            // assign the server as the new primary
            primaryServers[cluster] = server;
            primaryReassigned = true;
            break;
        }
    }
    if (primaryReassigned)
    {
        std::cout << "[REASSIGN PRIMARY] Sending a message to all in cluster" << std::endl;
        Communicator::setPrimaryServer(primaryServers[cluster], client);
    }
    // log that no server alive in cluster
    std::cout << "[REASSIGN PRIMARY] NO SERVER ALIVE IN CLUSTER" << std::endl;
}

// Create and initialize key mappings
void Maps::createKeyMappings(int nClusters)
{
    keyMappings.clear();

    // Letters to distribute (A-Z)
    char startLetter = 'A';
    char endLetter = 'Z';

    // Calculate the range for each cluster
    int totalLetters = endLetter - startLetter + 1;
    int clusterRange = std::ceil(static_cast<double>(totalLetters) / nClusters);

    // Assign ranges to clusters
    for (int clusterIndex = 0; clusterIndex < nClusters; ++clusterIndex)
    {
        char rangeStart = startLetter + clusterIndex * clusterRange;
        char rangeEnd = std::min(static_cast<int>(rangeStart + clusterRange - 1), static_cast<int>(endLetter));

        keyMappings[clusterIndex] = std::make_tuple(rangeStart, rangeEnd);

        if (rangeEnd == endLetter)
            break;
    }
}

// Assign servers to clusters
void Maps::assignClusters(const std::vector<std::string> &servers, int clusterSize)
{
    serverClusterMap.clear();

    for (size_t i = 0; i < servers.size(); ++i)
    {
        // Determine the cluster assignment for the server
        int clusterIndex = i / clusterSize;
        serverClusterMap[servers[i]] = clusterIndex;
        // if clusters doesn't have the clusterIndex, add it
        if (clusters.find(clusterIndex) == clusters.end())
        {
            clusters[clusterIndex] = std::vector<std::string>();
        }
        clusters[clusterIndex].push_back(servers[i]);
        // print the server and cluster
        std::cout << "Server: " << servers[i] << " Cluster: " << clusterIndex << std::endl;
    }
    // iterate over the clusters and print the servers in each cluster
    for (const auto &[cluster, servers] : clusters)
    {
        std::cout << "[CLUSTER NUMBER]: " << cluster << std::endl;
        for (const auto &server : servers)
        {
            std::cout << "[SERVER]: " << server << std::endl;
        }
    }
}

// Getter for keyMappings
std::unordered_map<int, std::tuple<char, char>> Maps::getKeyMappings()
{
    return keyMappings;
}

// Getter for serverClusterMap
std::unordered_map<std::string, int> Maps::getServerClusterMap()
{
    return serverClusterMap;
}

// Getter for primaryServers
std::unordered_map<int, std::string> Maps::getPrimaryServers()
{
    return primaryServers;
}
// setter for clusters
void Maps::setClusters(std::unordered_map<int, std::vector<std::string>> clusters)
{
    Maps::clusters = clusters;
}
// getter for clusters
std::unordered_map<int, std::vector<std::string>> Maps::getClusters()
{
    // print the clusters
    for (const auto &[cluster, servers] : clusters)
    {
        std::cout << "[CLUSTER NUMBER IN GET CLUSTERS]: " << cluster << std::endl;
        for (const auto &server : servers)
        {
            std::cout << "[SERVER IN GET CLUSTERS]: " << server << std::endl;
        }
    }
    return clusters;
}
// remove server from clusters if not already removed
void Maps::removeServerFromCluster(const std::string &server, int cluster)
{
    auto &clusterServers = clusters[cluster];
    auto it = std::find(clusterServers.begin(), clusterServers.end(), server);
    if (it != clusterServers.end())
    {
        clusterServers.erase(it);
    }
}
// add server to cluster if not already present
void Maps::addServerToCluster(const std::string &server, int cluster)
{
    auto &clusterServers = clusters[cluster];
    if (std::find(clusterServers.begin(), clusterServers.end(), server) == clusterServers.end())
    {
        clusterServers.push_back(server);
    }
}