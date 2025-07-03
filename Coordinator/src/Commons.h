#ifndef COMMONS_H
#define COMMONS_H

#include <unordered_map>
#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Client.h"
class Maps
{
private:
    static std::unordered_map<int, std::tuple<char, char>> keyMappings;
    static std::unordered_map<std::string, int> serverClusterMap;
    // TODO: change vector to set
    static std::unordered_map<int, std::vector<std::string>> clusters;
    static std::unordered_map<int, std::string> primaryServers;

public:
    // Create and initialize key mappings
    static void createKeyMappings(int nClusters);

    // Create and initialize key mappings
    static void assignPrimary();
    // Reassign primary server
    static void reassignPrimary(std::string oldserver, Client &client);
    // Assign servers to clusters
    static void assignClusters(const std::vector<std::string> &servers, int clusterSize);

    // Getters for the mappings
    static std::unordered_map<int, std::tuple<char, char>> getKeyMappings();
    static std::unordered_map<std::string, int> getServerClusterMap();
    static std::unordered_map<int, std::string> getPrimaryServers();
    // Getters for the clusters
    static std::unordered_map<int, std::vector<std::string>> getClusters();
    // Setter for the clusters
    static void setClusters(std::unordered_map<int, std::vector<std::string>> clusters);
    // Remove or add server to cluster
    static void addServerToCluster(const std::string &server, int cluster);
    static void removeServerFromCluster(const std::string &server, int cluster);
};

class Commons
{
private:
    static std::vector<std::string> servers;
    // server status map data structure
    static std::unordered_map<std::string, bool> serverStatus;
    // failed to notify primary about these servers:
    static std::unordered_map<std::string, bool> failedServerNotifications;

public:
    // Read servers from Config file
    static void readServerConfig(const std::string &configFile);
    static std::vector<std::string> getServers();
    static bool singleHeartbeat(Client &client, std::string &server);
    static void heartbeatChecker(Client &client, std::vector<std::string> &servers);
    static void retryServer(Client &client, std::string &server);
    // getter for serverStatus
    static std::unordered_map<std::string, bool> getServerStatus();
    // setter for serverStatus
    static void setServerStatus(std::string server, bool status);
    // setter for failedServerNotifications
    static void setFailedServerNotifications(std::string server, bool status);
    // getter for failedServerNotifications
    static std::unordered_map<std::string, bool> getFailedServerNotifications();
};

#endif // COMMONS_H
