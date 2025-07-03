#include "ServiceLogic.h"
#include "SharedContext.h"
#include "InterServerLogic.h"
#include "ReplicationUtils.h"
#include "FileWriteUtils.h"
#include "MiscUtils.h"
#include "Constants.h"
#include <filesystem>
#include "checkpoint.h"
#include "Logger.h"
#include <memory>
#include "RecoveryUtils.h"

/// RPC Call for PUT Operation
grpc::Status ServiceLogic::Put(grpc::ServerContext *context, const PutArgs *args, PutReply *reply)
{
    UpdateArgs updateArgs;

    if (sharedContext_->getServerManager()->get_status() == false)
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is down");
    }
    constructUpdateArgs(updateArgs, args->row(), args->column(), args->value(), "", PUT);

    // Prepare an UpdateReply object
    UpdateReply updateReply;
    grpc::ClientContext clientContext;

    grpc::Status status = sharedContext_->getServerManager()->get_primary_stub()->callPrimary(&clientContext, updateArgs, &updateReply);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[PUT] primary server is %s", sharedContext_->getServerManager()->get_primary_server()) << endl;
    if (!status.ok())
    {
        ABSL_LOG(ERROR) << "Failed to forward request to primary server. Error: "
                        << status.error_message() << " (code: " << status.error_code() << ")";
        return status; // Return the failure status to the client
    }

    if (updateReply.status() == FAILURE)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to update all servers");
    }
    // int32_t currentTabletIndex = sharedContext_->getTabletManager()->get_current_tablet_index();
    // int32_t indexOfKey = sharedContext_->getTabletManager()->fetchTabletIndexOfKey(args->row());

    // if (currentTabletIndex != indexOfKey)
    // {

    // }

    // std::filesystem::path currentPath = std::filesystem::current_path();
    // std::filesystem::path logFilePath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".txt");
    // logOperation(logFilePath.string(), PUT, args->row(), args->column(), args->value());

    // serializeBigTable(sharedContext_->getTable(), currentPath / snapshotPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address()) + ".bin"));

    // sharedContext_->emptyTableMemory();
    // sharedContext_->createTable();
    // loadTableToBigTable(currentPath / snapshotPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address()) + ".bin"), sharedContext_->getTable());

    return grpc::Status::OK;
}

/// RPC Call for GET Operation
grpc::Status ServiceLogic::Get(grpc::ServerContext *context, const GetArgs *args, GetReply *reply)
{
    std::string value;

    if (sharedContext_->getServerManager()->get_status() == false)
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is down");
    }
    int indexOfKey = sharedContext_->getTabletManager()->fetchTabletIndexOfKey(args->row());
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GET] Index of key is %d", indexOfKey) << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GET] Current tablet index is %d", sharedContext_->getTabletManager()->get_current_tablet_index()) << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GET] Row and Column: %s, %s", args->row(), args->column()) << endl;
    if (indexOfKey != sharedContext_->getTabletManager()->get_current_tablet_index())
    {
        grpc::ClientContext clientContext;
        ReadEvictArgs readEvictArgs;
        readEvictArgs.set_checkpointtabletno(sharedContext_->getTabletManager()->get_current_tablet_index());
        readEvictArgs.set_loadtabletno(indexOfKey);
        ReadEvictReply readEvictReply;
        grpc::Status status = sharedContext_->getServerManager()->get_primary_stub()->checkPointAndEvictForRead(&clientContext, readEvictArgs, &readEvictReply);
        if (!status.ok())
        {
            return status;
        }
    }

    // Call BigTable's GET function to retrieve the value
    if (sharedContext_->get(args->row(), args->column(), value))
    {
        // log first 100 characters of value
        logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GET] Key: %s, Column: %s, Value: %s", args->row(), args->column(), value.substr(0, 100)) << endl;

        reply->set_value(value); // Set the retrieved value in the response
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GET] Key: %s, Column: %s, Value: %s", args->row(), args->column(), value);
        return grpc::Status::OK;
    }
    else
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Key or column not found");
    }
}

/// RPC Call for CPUT Operation
grpc::Status ServiceLogic::CPut(grpc::ServerContext *context, const CPutArgs *args, CPutReply *reply)
{
    UpdateArgs updateArgs;
    constructUpdateArgs(updateArgs, args->row(), args->column(), args->value1(), args->value2(), CPUT);
    UpdateReply updateReply;
    grpc::ClientContext clientContext;

    if (sharedContext_->getServerManager()->get_status() == false)
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is down");
    }

    grpc::Status status = sharedContext_->getServerManager()->get_primary_stub()->callPrimary(&clientContext, updateArgs, &updateReply);

    if (!status.ok())
    {
        ABSL_LOG(ERROR) << "Failed to forward request to primary server. Error: "
                        << status.error_message() << " (code: " << status.error_code() << ")";
        return status; // Return the failure status to the client
    }

    return grpc::Status::OK;
}

/// RPC Call for Delete Operation
grpc::Status ServiceLogic::Del(grpc::ServerContext *context, const DelArgs *args, DelReply *reply)
{

    if (sharedContext_->getServerManager()->get_status() == false)
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is down");
    }
    UpdateArgs updateArgs;
    constructUpdateArgs(updateArgs, args->row(), args->column(), "", "", DELETE);
    UpdateReply updateReply;
    grpc::ClientContext clientContext;

    grpc::Status status = sharedContext_->getServerManager()->get_primary_stub()->callPrimary(&clientContext, updateArgs, &updateReply);

    return grpc::Status::OK;
}

/// RPC Call for Kill Operation from Admin Console for soft shutdown of server
grpc::Status ServiceLogic::Kill(grpc::ServerContext *context, const KillArgs *args, KillReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[KILL] Received request to kill server") << flush;
    std::cout << "SharedContext address in Kill: " << sharedContext_.get() << std::endl;

    sharedContext_->getServerManager()->set_status(false);
    sharedContext_->getServerManager()->set_primary_server("");
    bool clearTabs = sharedContext_->getTabletManager()->clearTablets();
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("Checking Status ") << sharedContext_->getServerManager()->get_status() << flush;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[KILL] Server is shutting down") << flush;
    // response->set_message("Server is shutting down.");
    return grpc::Status::OK;
}

// RPC Call for Raw Data Fetch
grpc::Status ServiceLogic::FetchKeys(grpc::ServerContext *context, const KeyArgs *args, KeyReply *reply)
{

    if (sharedContext_->getServerManager()->get_status() == false)
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Server is down");
    }
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Received request to fetch keys") << endl
                                                                        << flush;
    unordered_map<string, vector<string>> keys;
    shared_ptr<BigTable> tempTable = std::make_shared<BigTable>();
    for (int i = 0; i < 3; i++)
    {
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address())) / "snapshots" / (to_string(i) + ".bin");
        std::filesystem::path logFilePath = currentPath / serverRecoveryPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address())) / "logs" / (to_string(i) + ".txt");
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "[FETCHKEYS] Snapshot file path: " << snapshotFilePath.string() << endl
        //                                                                     << flush;
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "[FETCHKEYS] Log file path: " << logFilePath.string() << endl;
        loadTableToBigTable(snapshotFilePath.string(), tempTable);
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Loaded snapshot for tablet %d", i) << endl
        //                                                                     << flush;
        readLogRecords(logFilePath.string(), tempTable);
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Loaded logs for tablet %d", i) << endl
        //                                                                     << flush;
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Loaded snapshot and logs for tablet %d", i) << endl
        //                                                                     << flush;
        unordered_map<string, RowData> *table = tempTable->getTablePointer();
        for (auto it = table->begin(); it != table->end(); it++)
        {
            const unordered_map<string, string> *columns = it->second.getRowPointer();
            vector<string> columnNames;
            for (auto it2 = columns->begin(); it2 != columns->end(); it2++)
            {
                // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Key: %s, Column: %s", it->first, it2->first) << endl
                //                                                                     << flush;
                columnNames.push_back(it2->first);
            }
            keys[it->first] = columnNames;
        }
    }
    for (const auto &[key, columnNames] : keys)
    {
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHKEYS] Key: %s", key) << endl
        //                                                                     << flush;
        CustomValue *customValue = &(*reply->mutable_my_map())[key];
        for (const auto &columnName : columnNames)
        {
            customValue->add_values(columnName);
        }
    }
    return grpc::Status::OK;

    // deserialize all 3 tablets and get the keys and columns
}

// RPC Call for Revive Operation from Admin Console for reviving server
grpc::Status ServiceLogic::Revive(grpc::ServerContext *context, const ReviveArgs *args, ReviveReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[REVIVE] Received request to revive server") << endl
                                                                        << flush;
    sharedContext_->getServerManager()->set_status(true);
    recovery(sharedContext_);
    return grpc::Status::OK;
}