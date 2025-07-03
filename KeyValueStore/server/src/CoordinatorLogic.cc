#include "CoordinatorLogic.h"

using namespace std;

// RPC Call for sending server status or hearbeat
grpc::Status CoordinatorLogic::getStatus(grpc::ServerContext *context, const StatusArgs *args, StatusReply *reply)
{

    if (!sharedContext_->getServerManager()->get_status())
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is not ready");
    }

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[STATUS] Received request for status");
    return grpc::Status::OK;
}

// RPC Call for setting primary server in the cluster
grpc::Status CoordinatorLogic::setPrimaryServer(grpc::ServerContext *context, const SetPrimaryServerArgs *args, SetPrimaryServerReply *reply)
{
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[PRIMARYASSIGNMENT] Received request for primary assignment");

    cout << "[PRIMARYASSIGNMENT] Received request for primary assignment" << flush << endl;
    cout << "Primary Server: is " << args->primaryserver() << flush << endl;
    sharedContext_->getServerManager()->set_primary_server(args->primaryserver());
    cout << "SharedContext address in setPrimaryServer: " << sharedContext_.get() << flush << endl;
    cout << "Primary Server: " << sharedContext_->getServerManager()->get_primary_server() << flush << endl;

    if (sharedContext_->getServerManager()->get_address() == args->primaryserver())
    {
        // call fetchAllAddress
        grpc::ClientContext clientContextForCoordinator;
        AllAddressReply allAddressReply;
        AllAddressArgs allAddressArgs;
        vector<string> allAddresses;
        allAddressArgs.set_ipaddressport(sharedContext_->getServerManager()->get_address());
        grpc::Status status = sharedContext_->getServerManager()->get_coordinator_stub()->fetchAllAddress(&clientContextForCoordinator, allAddressArgs, &allAddressReply);
        if (!status.ok())
        {
            logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[PRIMARYASSIGNMENT] Failed to fetch all addresses from coordinator") << flush << endl;
        }
        else
        {
            allAddresses.assign(allAddressReply.addresses().begin(), allAddressReply.addresses().end());
            sharedContext_->getServerManager()->clear_servers();

            for (auto server : allAddresses)
            {
                logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[PRIMARYASSIGNMENT] Adding server %s to server manager", server) << flush << endl;
                sharedContext_->getServerManager()->add_server(server);
            }
            logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[PRIMARYASSIGNMENT] All addresses received from coordinator") << flush << endl;
        }
    }

    return grpc::Status::OK;
}

// RPC Call to inform the primary server that a server is down
grpc::Status CoordinatorLogic::serverDown(grpc::ServerContext *context, const ServerDownArgs *args, ServerDownReply *reply)
{
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[SERVERDOWN] Received request for server down") << args->ipaddressport();
    cout << "[SERVERDOWN] Received request for server down" << args->ipaddressport() << flush << endl;
    cout << "SharedContext address in serverDown: " << sharedContext_.get() << flush << endl;
    sharedContext_->getServerManager()->remove_server(args->ipaddressport());
    return grpc::Status::OK;
}

// RPC Call to inform the primary server that a server is up
grpc::Status CoordinatorLogic::serverUp(grpc::ServerContext *context, const ServerUpArgs *args, ServerUpReply *reply)
{
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[SERVERUP] Received request for server up") << args->ipaddressport();
    cout << "[SERVERUP] Received request for server up" << args->ipaddressport() << flush << endl;
    sharedContext_->getServerManager()->findOrAddServer(args->ipaddressport());

    return grpc::Status::OK;
}