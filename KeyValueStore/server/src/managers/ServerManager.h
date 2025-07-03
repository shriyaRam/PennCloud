#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include "proto/interserver.grpc.pb.h"
#include "proto/interserver.pb.h"
#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
using namespace std;

class ServerManager
{
public:
    ServerManager(const string &address) : address(address) {}
    const string &get_primary_server();
    set<string> &get_servers();
    void set_primary_server(string server);
    void add_server(string server);
    void set_address(string address);
    const string &get_address();
    InterServer::Stub *get_primary_stub();
    Coordinator::Stub *get_coordinator_stub();
    void set_checkPointVersion(int version);
    int get_checkPointVersion();
    void remove_server(string server);
    void findOrAddServer(string server);
    int currentWrites = 0;
    int32_t currentSerial = 0;

    void setCoordinatorServer(string address)
    {
        coordinatorAddress = address;
        coordinatorChannel_ = grpc::CreateChannel(coordinatorAddress, grpc::InsecureChannelCredentials());
        coordinatorStub_ = Coordinator::NewStub(coordinatorChannel_);
    }

    string get_coordinator_address();
    void set_status(bool status);
    bool get_server_status(string address);
    bool get_status();

    void clear_servers()
    {
        servers.clear();
    }

private:
    set<string> servers;
    string primary_server;
    string address;
    shared_ptr<grpc::Channel> primaryChannel_;
    std::unique_ptr<InterServer::Stub> primaryStub_;
    int checkPointVersion;
    string coordinatorAddress;
    shared_ptr<grpc::Channel> coordinatorChannel_;
    std::unique_ptr<Coordinator::Stub> coordinatorStub_;
    bool status = false;
    unordered_map<string, bool> serverStatus;
};