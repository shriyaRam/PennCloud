#include "ServerManager.h"
#include "proto/interserver.grpc.pb.h"
#include "proto/interserver.pb.h"
#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
#include "Logger.h"
const string &ServerManager::get_primary_server()
{
    return primary_server;
}

set<string> &ServerManager::get_servers()
{
    return servers;
}

const string &ServerManager::get_address()
{
    return address;
}

void ServerManager::set_primary_server(string server)
{
    if (server == "")
    {
        primary_server = "";
        return;
    }

    primary_server = server;
    primaryChannel_ = grpc::CreateChannel(primary_server, grpc::InsecureChannelCredentials());
    primaryStub_ = InterServer::NewStub(primaryChannel_);
}

InterServer::Stub *ServerManager::get_primary_stub()
{
    return primaryStub_.get();
}

void ServerManager::add_server(string server)
{
    servers.insert(server);
    serverStatus[server] = true;
}

void ServerManager::set_checkPointVersion(int version)
{
    checkPointVersion = version;
}

int ServerManager::get_checkPointVersion()
{
    return checkPointVersion;
}

void ServerManager::remove_server(string server)
{
    // servers.erase(server);
    cout << "Removing server " << server << endl;
    serverStatus[server] = false;
    cout << "Server status is " << serverStatus[server] << endl;
}

void ServerManager::findOrAddServer(string server)
{
    servers.insert(server);
    serverStatus[server] = true;
}

Coordinator::Stub *ServerManager::get_coordinator_stub()
{
    return coordinatorStub_.get();
}

string ServerManager::get_coordinator_address()
{
    return coordinatorAddress;
}

void ServerManager::set_status(bool status)
{
    this->status = status;
}

bool ServerManager::get_status()
{
    return status;
}

bool ServerManager::get_server_status(string address)
{
    if (serverStatus.find(address) == serverStatus.end())
    {
        return false;
    }
    return serverStatus[address];
}