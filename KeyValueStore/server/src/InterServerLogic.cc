#include "InterServerLogic.h"
#include "MiscUtils.h"
#include <string>
#include <vector>
#include "FileWriteUtils.h"
#include "checkpoint.h"
#include "checkpoint.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include "Logger.h"
#include "Logger.h"
#include <thread>
#include "RecoveryUtils.h"

#include "Constants.h" // Retained from main

using namespace std;

// RPC Call for Primary Server to initiate a write operation
grpc::Status InterServerLogic::callPrimary(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CALLPRIMARY] Received request to update primary server");
    string row = args->row();
    string column = args->column();
    string value1 = args->value1();
    string value2 = args->value2();
    string operation = args->operation();

    /// TODO: Implement Sequencing of operations to handle concurrent requests

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CALLPRIMARY] Key: %s, Column: %s, Value: %s, Operation: %s", row, column, value1, operation);

    string primaryServer = sharedContext_->getServerManager()->get_primary_server();
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    std::filesystem::path logFilePath = currentPath / "../../server/src/replicationlogs/" / (sanitizedAddress + ".txt");
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Primary Working Directory: " << logFilePath << std::endl;
    int32_t currentSerial = sharedContext_->getServerManager()->currentSerial;
    // logTransaction(logFilePath.string(), currentSerial, COMMIT);

    vector<string> ackServers;
    grpc::Status writeStatus = grpc::Status::OK;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "before send To Server " << primaryServer << endl;

    // Forward request to other servers
    // for (auto server : sharedContext_->getServerManager()->get_servers())
    // {
    //     if (server != sharedContext_->getServerManager()->get_primary_server())
    //     {
    //         auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
    //         auto stub = InterServer::NewStub(channel);
    //         UpdateArgs updateArgs;
    //         updateArgs.set_row(row);
    //         updateArgs.set_column(column);
    //         updateArgs.set_value1(value1);
    //         updateArgs.set_value2(value2);
    //         updateArgs.set_operation(operation);
    //         updateArgs.set_serial(currentSerial);

    //         UpdateReply updateReply;
    //         grpc::ClientContext clientContext;
    //         grpc::Status status = stub->askServer(&clientContext, updateArgs, &updateReply);

    //         if (!status.ok())
    //         {
    //             writeStatus = status;
    //             ABSL_LOG(ERROR) << "Failed to forward request to server: " << server
    //                             << ". Error: " << status.error_message();
    //             break;
    //         }
    //         ackServers.push_back(server);
    //     }
    // }

    // if (writeStatus.ok())
    // {
    // sharedContext_->getServerManager()->currentSerial += 1;
    UpdateArgs updateArgs;
    updateArgs.set_row(row);
    updateArgs.set_column(column);
    updateArgs.set_value1(value1);
    updateArgs.set_value2(value2);
    updateArgs.set_operation(operation);
    updateArgs.set_serial(currentSerial);

    // do hash of the row to see which tablet it belongs to
    int indexOfKey = sharedContext_->getTabletManager()->fetchTabletIndexOfKey(row);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Index of key is " << indexOfKey << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current tablet index in InterServerLogic " << sharedContext_->getTabletManager()->get_current_tablet_index() << endl;
    // now we have the index of the tablet where the key belongs, check if the current tablet is the same as the tablet where the key belongs
    if (sharedContext_->getTabletManager()->get_current_tablet_index() != indexOfKey && sharedContext_->getTabletManager()->get_current_tablet_index() != -1)
    {
        // if not, then we need to checkpoint the current tablet and switch to the tablet where the key belongs
        for (auto server : sharedContext_->getServerManager()->get_servers())
        {

            if (sharedContext_->getServerManager()->get_server_status(server) == false)
            {
                continue;
            }
            cout << "Checkpointing server: " << server << endl;
            auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
            auto stub = InterServer::NewStub(channel);
            CheckPointArgs checkPointArgs;
            checkPointArgs.set_tabletno(sharedContext_->getTabletManager()->get_current_tablet_index());
            logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Checkpointing tablet no: " << sharedContext_->getTabletManager()->get_current_tablet_index() << endl;
            CheckPointReply checkPointReply;
            grpc::ClientContext clientContext;
            grpc::Status st = stub->checkpointing(&clientContext, checkPointArgs, &checkPointReply);
            if (!st.ok())
            {
                writeStatus = st;
                ABSL_LOG(ERROR) << "Failed to checkpoint tablet: " << sharedContext_->getTabletManager()->get_current_tablet_index()
                                << ". Error: " << st.error_message();
                break;
            }
        }
    }

    for (auto server : sharedContext_->getServerManager()->get_servers())
    {
        if (sharedContext_->getServerManager()->get_server_status(server) == false)
        {
            continue;
        }
        auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
        auto stub = InterServer::NewStub(channel);
        LoadTabletArgs loadTabletArgs;
        loadTabletArgs.set_tabletno(indexOfKey);
        LoadTabletReply loadTabletReply;
        grpc::ClientContext clientContext2;
        logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Loading tablet no after checkpointing: " << loadTabletArgs.tabletno() << endl;
        std::cout << "Handling loadNewTablet before on thread: " << std::this_thread::get_id() << std::endl;

        grpc::Status loadStatus = stub->loadNewTablet(&clientContext2, loadTabletArgs, &loadTabletReply);
        if (!loadStatus.ok())
        {
            writeStatus = loadStatus;
            ABSL_LOG(ERROR) << "Failed to load tablet: " << indexOfKey
                            << ". Error: " << loadStatus.error_message();
            break;
        }
    }

    // sharedContext_->getTabletManager()->emptyTableMemory();
    // sharedContext_->getTabletManager()->emptyTableMemory(sharedContext_->getTabletManager()->get_current_tablet_index());
    // now we switch to the tablet where the key belongs, but first we need to load the checkpoint of the tablet
    // std::filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address())) / "snapshots" / (to_string(indexOfKey) + ".bin");
    // sharedContext_->getTabletManager()->set_current_tablet_index(indexOfKey);
    // shared_ptr<BigTable> tablet = std::make_shared<BigTable>();
    // loadTableToBigTable(snapshotFilePath.string(), tablet);
    // sharedContext_->getTabletManager()->set_current_tablet(tablet);
    // sharedContext_->getTabletManager()->

    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Sending to primary first" << endl
                                                                        << flush;

    auto channel = grpc::CreateChannel(primaryServer, grpc::InsecureChannelCredentials());
    auto stub = InterServer::NewStub(channel);
    UpdateReply primaryUpdateReply;
    grpc::ClientContext primaryClientContext;
    grpc::Status primaryStatus = stub->sendToServer(&primaryClientContext, updateArgs, &primaryUpdateReply);

    if (!primaryStatus.ok())
    {
        writeStatus = primaryStatus;
        ABSL_LOG(ERROR) << "Failed to write request to primary server: " << primaryServer
                        << ". Error: " << primaryStatus.error_message();

        return writeStatus;
    }
    else
    {
        logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Successfully updated primary server" << flush << endl;
        for (auto server : sharedContext_->getServerManager()->get_servers())
        {
            if (server == sharedContext_->getServerManager()->get_primary_server())
            {
                continue;
            }
            if (sharedContext_->getServerManager()->get_server_status(server) == false)
            {
                continue;
            }
            auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
            auto stub = InterServer::NewStub(channel);

            UpdateReply updateReply;
            grpc::ClientContext clientContext;
            logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "tablet no before calling each server for operation: " << sharedContext_->getTabletManager()->get_current_tablet_index() << endl;
            grpc::Status status = stub->sendToServer(&clientContext, updateArgs, &updateReply);

            if (!status.ok())
            {
                writeStatus = status;
                ABSL_LOG(ERROR) << "Failed to write request to server: " << server
                                << ". Error: " << status.error_message();
                break;
            }
        }
    }

    reply->set_status(SUCCESS);
    reply->set_message("Successfully updated all servers");
    // sharedContext_->getServerManager()->currentWrites += 1;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current writes: " << sharedContext_->getServerManager()->currentWrites << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current tablet index in InterServerLogic after operation is " << sharedContext_->getTabletManager()->get_current_tablet_index() << endl
                                                                        << flush;
    if (sharedContext_->getServerManager()->currentWrites == numOfWrites && sharedContext_->getTabletManager()->get_current_tablet_index() != -1)
    {
        // sharedContext_->getServerManager()->currentWrites = 0;
        for (auto server : sharedContext_->getServerManager()->get_servers())
        {
            if (sharedContext_->getServerManager()->get_server_status(server) == false)
            {
                continue;
            }
            auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
            auto stub = InterServer::NewStub(channel);
            CheckPointArgs checkPointArgs;
            checkPointArgs.set_tabletno(sharedContext_->getTabletManager()->get_current_tablet_index());
            CheckPointReply checkPointReply;
            grpc::ClientContext clientContext;
            stub->checkpointing(&clientContext, checkPointArgs, &checkPointReply);
        }
        // sharedContext_->getServerManager()->set_checkPointVersion(sharedContext_->getServerManager()->get_checkPointVersion() + 1);
        // std::filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "snapshots" / (to_string(i) + ".bin");
        // serializeBigTable(sharedContext_->getTabletManager()->get_current_tablet(), snapshotFilePath);
    }
    // }
    // else
    // {
    //     for (auto server : ackServers)
    //     {
    //         auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
    //         auto stub = InterServer::NewStub(channel);
    //         UpdateArgs updateArgs;
    //         updateArgs.set_row(row);
    //         updateArgs.set_column(column);
    //         updateArgs.set_value1(value1);
    //         updateArgs.set_value2(value2);
    //         updateArgs.set_operation(ROLLBACK);

    //         UpdateReply updateReply;
    //         grpc::ClientContext clientContext;
    //         stub->rollBackServer(&clientContext, updateArgs, &updateReply);
    //     }

    //     reply->set_status(FAILURE);
    //     reply->set_message("Failed to update all servers");
    // }

    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CALLPRIMARY] Finished processing request");

    return writeStatus;
}

// RPC Call for Primary Server to initiate a write operation for 2PC
grpc::Status InterServerLogic::askServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[ASKSERVER] Received request to update server");
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);

    std::filesystem::path logFilePath = currentPath / "../../server/src/replicationlogs/" / (sanitizedAddress + ".txt");
    logOperation(logFilePath.string(), args->operation(), args->row(), args->column(), args->value1(), args->value2());

    return grpc::Status::OK;
}

// RPC Call for performing write operation on all servers
grpc::Status InterServerLogic::sendToServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[SENDTOALL] Processing request to update servers");
    string row = args->row();
    string column = args->column();
    string value1 = args->value1();
    string operation = args->operation();

    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Inside sendToServer" << endl;
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Row: " << row << " Column: " << column << " Value: " << value1 << " Operation: " << operation << endl;

    if (operation == PUT)
    {
        sharedContext_->put(row, column, value1);
    }
    else if (operation == DELETE)
    {
        sharedContext_->del(row, column);
    }
    else if (operation == CPUT)
    {
        sharedContext_->cput(row, column, value1, args->value2());
    }
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Operation done" << endl;
    pthread_mutex_t *currentTabletIndexMutex = sharedContext_->getCurrentTabletIndexMutex();
    pthread_mutex_lock(currentTabletIndexMutex);
    int currentTabletIndex = sharedContext_->getTabletManager()->get_current_tablet_index();
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current tablet index inside sendToServer is " << currentTabletIndex << endl;
    int indexOfKey = sharedContext_->getTabletManager()->fetchTabletIndexOfKey(row);

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[SENDTOALL] Update completed for Row: %s, Column: %s, Value: %s", row, column, value1);
    std::filesystem::path currentPath = std::filesystem::current_path();

    std::filesystem::path logFilePath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address())) / "logs" / (to_string(currentTabletIndex) + ".txt");
    std::filesystem::path metaFilePath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(sharedContext_->getServerManager()->get_address())) / "logs" / (to_string(currentTabletIndex) + ".meta");
    pthread_mutex_t *logFileMutex = sharedContext_->getLogFileMutex(currentTabletIndex);
    pthread_mutex_lock(logFileMutex);
    // size_t lineCount = getCurrentLineCount(metaFilePath.string());
    // logOperation(logFilePath.string(), operation, row, column, value1, args->value2(), args->serial());
    writeLogRecord(logFilePath.string(), operation, row, column, value1, args->value2());
    // updateLineCount(metaFilePath.string(), lineCount + 1);
    pthread_mutex_unlock(logFileMutex);
    pthread_mutex_unlock(currentTabletIndexMutex);

    sharedContext_->getServerManager()->currentWrites++;

    return grpc::Status::OK;
}

// RPC Call for performing rollback operation on all servers after 2PC Failure
grpc::Status InterServerLogic::rollBackServer(grpc::ServerContext *context, const UpdateArgs *args, UpdateReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[ROLLBACK] Performing rollback operation");
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    std::filesystem::path logFilePath = currentPath / "../../server/src/replicationlogs/" / (sanitizedAddress + ".txt");
    int32_t currentSerial = args->serial();
    // deleteLastLineEfficiently(logFilePath.string());
    logTransaction(logFilePath.string(), currentSerial, ABORT);

    return grpc::Status::OK;
}

// RPC Call for send checkpoint version
grpc::Status InterServerLogic::sendCheckPoint(grpc::ServerContext *context, const CheckPointArgs *args, CheckPointReply *reply)
{
    pthread_mutex_t *snapShotMutex = sharedContext_->getSnapshotMutex(args->tabletno());
    pthread_mutex_lock(snapShotMutex);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CHECKPOINT] Received request to send checkpoint");
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    // string tabletNo = to_string(args->tabletno());
    // int32_t checkPointVersion = sharedContext_->getTabletManager()->get_tablet_version(args->tabletno());
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Checkpoint version is " << checkPointVersion << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Tablet no is " << args->tabletno() << endl;
    // reply->set_checkpointversion(checkPointVersion);

    std::filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "snapshots" / (to_string(args->tabletno()) + ".bin");
    int32_t checkPointVersion = getCheckpointVersion(snapshotFilePath.string());
    reply->set_checkpointversion(checkPointVersion);
    // std::filesystem::path logFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(args->tabletno()) + ".txt");
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Snapshot file path is " << snapshotFilePath << endl;
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Log file path is " << logFilePath << endl;
    // reply->set_snapshotfilepath(snapshotFilePath.string());
    // reply->set_logfilepath(logFilePath.string());
    pthread_mutex_unlock(snapShotMutex);
    return grpc::Status::OK;
}

// RPC Call for initiating checkpointing
grpc::Status InterServerLogic::checkpointing(grpc::ServerContext *context, const CheckPointArgs *args, CheckPointReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CHECKPOINT] Received request to checkpoint") << endl;
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    // string tabletNo = to_string(args->tabletno());
    int32_t tabletNo = args->tabletno();
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Tablet no in checkpointing is " << tabletNo << endl;
    int32_t checkPointVersion = sharedContext_->getTabletManager()->get_tablet_version(tabletNo);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Checkpoint version in checkpointing is " << checkPointVersion << endl;
    pthread_mutex_t *snapShotMutex = sharedContext_->getSnapshotMutex(tabletNo);
    pthread_mutex_lock(snapShotMutex);
    serializeBigTable(sharedContext_->getTabletManager()->get_tablet(tabletNo), currentPath / serverRecoveryPath / (sanitizedAddress) / "snapshots" / (to_string(tabletNo) + ".bin"), checkPointVersion + 1);
    pthread_mutex_unlock(snapShotMutex);
    sharedContext_->getTabletManager()->set_tablet_version(args->tabletno(), checkPointVersion + 1);
    std::filesystem::path logFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(tabletNo) + ".txt");
    std::filesystem::path metaFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(tabletNo) + ".meta");
    pthread_mutex_t *logFileMutex = sharedContext_->getLogFileMutex(tabletNo);
    pthread_mutex_lock(logFileMutex);
    clearFile(logFilePath);
    // updateLineCount(metaFilePath.string(), 0);
    pthread_mutex_unlock(logFileMutex);
    reply->set_checkpointversion(checkPointVersion + 1);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Latest checkpoint version is " << checkPointVersion + 1 << endl;
    sharedContext_->getTabletManager()->set_tablet_version(args->tabletno(), checkPointVersion + 1);
    sharedContext_->getServerManager()->currentWrites = 0;
    return grpc::Status::OK;
}

// RPC Call for loading new tablet
grpc::Status InterServerLogic::loadNewTablet(grpc::ServerContext *context, const LoadTabletArgs *args, LoadTabletReply *reply)
{
    std::cout << "Handling loadNewTablet on thread: " << std::this_thread::get_id() << std::endl;

    pthread_mutex_t *currentTabletIndexMutex = sharedContext_->getCurrentTabletIndexMutex();
    pthread_mutex_lock(currentTabletIndexMutex);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[LOADTABLET] Received request to load new tablet");
    std::filesystem::path currentPath = std::filesystem::current_path();
    string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    string tabletNo = to_string(args->tabletno());
    std::filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "snapshots" / (tabletNo + ".bin");
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Snapshot file path in loadNewTablet is " << snapshotFilePath << endl;
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Tablet no in loadNewTablet is " << args->tabletno() << endl;
    shared_ptr<BigTable> tablet = std::make_shared<BigTable>();
    pthread_mutex_t *snapShotMutex = sharedContext_->getSnapshotMutex(args->tabletno());
    pthread_mutex_lock(snapShotMutex);
    loadTableToBigTable(snapshotFilePath.string(), tablet);
    pthread_mutex_unlock(snapShotMutex);
    // processLogFile(currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (tabletNo + ".txt"), tablet);
    readLogRecords(currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (tabletNo + ".txt"), tablet);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Tablet loaded successfully" << endl;
    sharedContext_->getTabletManager()->set_tablet(args->tabletno(), tablet);
    sharedContext_->getTabletManager()->set_current_tablet_index(args->tabletno());
    sharedContext_->getTabletManager()->set_current_tablet(tablet);
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current tablet index in loadNewTablet is " << sharedContext_->getTabletManager()->get_current_tablet_index() << endl;
    pthread_mutex_unlock(currentTabletIndexMutex);
    // sharedContext_->getTabletManager()->set_current_tablet_index(args->tabletno());
    return grpc::Status::OK;
}

// RPC Call for fetching current tablet index
grpc::Status InterServerLogic::fetchCurrentTabletIndex(grpc::ServerContext *context, const TabletIndexArgs *args, TabletIndexReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[FETCHTABLETINDEX] Received request to fetch current tablet index");
    reply->set_tabletindex(sharedContext_->getTabletManager()->get_current_tablet_index());
    return grpc::Status::OK;
}

// RPC Call for fetching missing logs
grpc::Status InterServerLogic::getMissingLogsFromPosition(grpc::ServerContext *context,
                                                          const LogPositionRequest *request,
                                                          LogPositionResponse *response)
{
    int64_t startPosition = request->start_position();
    int64_t chunkSize = CHUNK_SIZE;
    int32_t tabletNo = request->tabletno();

    // Construct the log file path
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    std::filesystem::path logFilePath = currentPath / serverRecoveryPath / sanitizedAddress / "logs" / (std::to_string(tabletNo) + ".txt");

    std::ifstream logFile(logFilePath.string(), std::ios::binary);
    if (!logFile.is_open())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Log file not found");
    }

    // Seek to the starting position
    logFile.seekg(startPosition, std::ios::beg);
    if (logFile.fail())
    {
        return grpc::Status(grpc::StatusCode::OUT_OF_RANGE, "Start position out of range");
    }

    // Read the chunk of data
    std::vector<char> buffer(chunkSize);
    logFile.read(buffer.data(), chunkSize);

    // Populate the response with the chunk data
    response->set_chunk_data(std::string(buffer.data(), logFile.gcount()));

    // Get the new position in the file
    int64_t currentPosition = logFile.tellg();
    if (currentPosition == -1) // If at EOF, tellg() returns -1
    {
        currentPosition = std::filesystem::file_size(logFilePath);
    }

    response->set_next_position(currentPosition);
    response->set_is_done(logFile.eof());

    logFile.close();
    return grpc::Status::OK;
}

// grpc::Status InterServerLogic::getMissingLogs(grpc::ServerContext *context, const LogLineRequest *args, LogLineResponse *reply)
// {
//     logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[GETMISSINGLOGS] Received request to get missing logs") << endl;
//     int64_t lastLineNumber = args->last_line_number();
//     std::filesystem::path currentPath = std::filesystem::current_path();
//     std::string address = sharedContext_->getServerManager()->get_address();
//     std::string sanitizedAddress = sanitizeAddress(address);
//     std::filesystem::path logFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(args->tabletno()) + ".txt");
//     logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Log file path in getMissingLogs is " << logFilePath << endl;
//     logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Last line number in getMissingLogs is " << lastLineNumber << endl;

//     std::ifstream logFile(logFilePath.string());
//     if (!logFile.is_open())
//     {
//         return grpc::Status(grpc::StatusCode::NOT_FOUND, "Log file not found");
//     }

//     std::string line;
//     int64_t currentLine = 0;
//     std::ostringstream dataStream;
//     size_t lineSize = 0;
//     const size_t maxLineSize = CHUNK_SIZE;

//     // Skip lines up to the last line number
//     while (currentLine < lastLineNumber && std::getline(logFile, line))
//     {
//         // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Skipping line: " << line << endl;
//         currentLine++;
//     }

//     // Add lines after the last line to the response
//     while (std::getline(logFile, line))
//     {
//         currentLine++;
//         // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Adding line: " << line << endl;
//         // if (!line.empty() && line.back() != '\n')
//         // {
//         //     // Add a newline only if it's missing
//         //     dataStream << line << "\n";
//         // }
//         // else
//         // {
//         // Append the line directly if newline is already included
//         dataStream << line;
//         // }
//         lineSize += line.size() + 1;
//         if (lineSize >= maxLineSize)
//         {
//             break;
//         }
//     }
//     // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Stream data in getMissingLogs is " << dataStream.str() << endl;
//     // Set the concatenated log data and the `is_done` flag
//     reply->set_data(dataStream.str());
//     reply->set_is_done(true);

//     logFile.close();
//     return grpc::Status::OK;
// }

// RPC Call for fetching checkpoint data
grpc::Status InterServerLogic::getCheckpointChunk(grpc::ServerContext *context,
                                                  const CheckpointChunkRequest *request,
                                                  CheckpointChunkResponse *response)
{
    int64_t chunkNumber = request->chunk_number();
    int64_t chunkSize = request->chunk_size();
    int32_t tabletNo = request->tabletno();

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    std::filesystem::path checkpointFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "snapshots" / (to_string(tabletNo) + ".bin");

    std::ifstream checkpointFile(checkpointFilePath.string(), std::ios::binary);
    if (!checkpointFile.is_open())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Checkpoint file not found");
    }

    // Calculate the offset based on the chunk number
    int64_t offset = chunkNumber * chunkSize;

    // Move the file pointer to the offset
    checkpointFile.seekg(offset, std::ios::beg);
    if (checkpointFile.fail())
    {
        return grpc::Status(grpc::StatusCode::OUT_OF_RANGE, "Offset out of range");
    }

    // Read the chunk of data
    std::vector<char> buffer(chunkSize);
    checkpointFile.read(buffer.data(), chunkSize);

    // Populate the response with the chunk data
    response->set_chunk_data(std::string(buffer.data(), checkpointFile.gcount()));

    // Indicate if this is the last chunk
    response->set_is_done(checkpointFile.eof());

    checkpointFile.close();
    return grpc::Status::OK;
}

// RPC Call for getting log chunks
grpc::Status InterServerLogic::getLogChunk(grpc::ServerContext *context,
                                           const CheckpointChunkRequest *request,
                                           CheckpointChunkResponse *response)
{
    int64_t chunkNumber = request->chunk_number();
    int64_t chunkSize = request->chunk_size();
    int32_t tabletNo = request->tabletno();

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string address = sharedContext_->getServerManager()->get_address();
    std::string sanitizedAddress = sanitizeAddress(address);
    std::filesystem::path checkpointFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(tabletNo) + ".txt");

    std::ifstream checkpointFile(checkpointFilePath.string(), std::ios::binary);
    if (!checkpointFile.is_open())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Log file not found");
    }

    // Calculate the offset based on the chunk number
    int64_t offset = chunkNumber * chunkSize;

    // Move the file pointer to the offset
    checkpointFile.seekg(offset, std::ios::beg);
    if (checkpointFile.fail())
    {
        return grpc::Status(grpc::StatusCode::OUT_OF_RANGE, "Offset out of range");
    }

    // Read the chunk of data
    std::vector<char> buffer(chunkSize);
    checkpointFile.read(buffer.data(), chunkSize);

    // Populate the response with the chunk data
    response->set_chunk_data(std::string(buffer.data(), checkpointFile.gcount()));

    // Indicate if this is the last chunk
    response->set_is_done(checkpointFile.eof());

    checkpointFile.close();
    return grpc::Status::OK;
}

// RPC call for checkpoint and eviction
grpc::Status InterServerLogic::checkPointAndEvictForRead(grpc::ServerContext *context, const ReadEvictArgs *args, ReadEvictReply *reply)
{
    logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("[CHECKPOINTANDEVICT] Received request to checkpoint and evict for read") << endl;
    CheckPointArgs checkPointArgs;
    checkPointArgs.set_tabletno(args->checkpointtabletno());
    if (args->checkpointtabletno() != -1)
    {

        for (auto server : sharedContext_->getServerManager()->get_servers())
        {

            if (sharedContext_->getServerManager()->get_server_status(server) == false)
            {
                continue;
            }
            auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
            auto stub = InterServer::NewStub(channel);
            CheckPointReply checkPointReply;
            grpc::ClientContext clientContext;
            grpc::Status status = stub->checkpointing(&clientContext, checkPointArgs, &checkPointReply);
            if (!status.ok())
            {
                return status;
            }
        }
    }
    LoadTabletArgs loadTabletArgs;
    loadTabletArgs.set_tabletno(args->loadtabletno());
    if (args->loadtabletno() != -1)
    {
        for (auto server : sharedContext_->getServerManager()->get_servers())
        {
            if (sharedContext_->getServerManager()->get_server_status(server) == false)
            {
                continue;
            }
            auto channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
            auto stub = InterServer::NewStub(channel);

            LoadTabletReply loadTabletReply;
            grpc::ClientContext clientContext;
            grpc::Status status = stub->loadNewTablet(&clientContext, loadTabletArgs, &loadTabletReply);
            if (!status.ok())
            {
                return status;
            }
        }
    }

    return grpc::Status::OK;
}
