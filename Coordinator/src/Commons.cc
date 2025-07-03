#include "Commons.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <cmath>
// All servers in the system
std::vector<std::string> Commons::servers;

// All server status in the system
std::unordered_map<std::string, bool> Commons::serverStatus;

// All servers that were not notified about to primary
std::unordered_map<std::string, bool> Commons::failedServerNotifications;

// setter for serverStatus
void Commons::setServerStatus(std::string server, bool status)
{
    serverStatus[server] = status;
}

// Function to read config file with list of servers in system
void Commons::readServerConfig(const std::string &configFile)
{
    std::ifstream file(configFile);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open the config file: " << configFile << std::endl;
        return;
    }

    std::cout << "Reading configuration file..." << std::endl;

    std::string line;
    while (std::getline(file, line))
    {
        servers.push_back(line);
        setServerStatus(line, false);
    }

    std::cout << "Finished reading configuration file." << std::endl;
}

std::vector<std::string> Commons::getServers()
{
    return servers;
}

// getter for serverStatus
std::unordered_map<std::string, bool> Commons::getServerStatus()
{
    ABSL_LOG(INFO) << "Getting server status within get function: " << std::endl;
    // print server status
    for (const auto &[server, status] : serverStatus)
    {
        ABSL_LOG(INFO) << "Server: " << server << " Status: " << status;
    }
    return serverStatus;
}

// setter for failedServerNotifications
void Commons::setFailedServerNotifications(std::string server, bool status)
{
    failedServerNotifications[server] = status;
}

// getter for failedServerNotifications
std::unordered_map<std::string, bool> Commons::getFailedServerNotifications()
{
    return failedServerNotifications;
}
