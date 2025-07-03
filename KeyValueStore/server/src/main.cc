#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <atomic>
#include "ServiceLogic.h"
#include "CoordinatorLogic.h"
#include "SharedContext.h"
#include "InterServerLogic.h"
#include "FileWriteUtils.h"
#include "RecoveryUtils.h"
#include "checkpoint.h"
#include "Constants.h"
#include <sys/types.h>
#include "MiscUtils.h"
#include <unistd.h>
#include "Logger.h"
// #include "ClearFiles.h"

#define DEFAULT_PORT "50051"

using namespace std;

namespace fs = std::filesystem;
void compareFiles(const std::string &file1, const std::string &file2)
{
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    if (!f1.is_open() || !f2.is_open())
    {
        std::cerr << "Error: Unable to open one or both files." << std::endl;
        return;
    }

    char ch1, ch2;
    int position = 0; // Position tracker
    bool hasDifference = false;

    while (true)
    {
        f1.get(ch1);
        f2.get(ch2);

        // If both streams reach EOF, break
        if (f1.eof() && f2.eof())
        {
            break;
        }

        // If one file ends before the other, report the difference
        if (f1.eof() || f2.eof())
        {
            std::cout << "Files differ in length starting at position " << position << std::endl;
            hasDifference = true;
            break;
        }

        // Compare characters
        if (ch1 != ch2)
        {
            std::cout << "Difference at position " << position
                      << ": file1='" << ch1 << "' file2='" << ch2 << "'" << std::endl;
            hasDifference = true;
        }

        position++;
    }

    if (!hasDifference)
    {
        std::cout << "The files are identical." << std::endl;
    }

    f1.close();
    f2.close();
}
// Function to clear file contents in a specific folder with a given file extension
void clearFilesInFolder(const fs::path &basePath, const std::string &subFolder, const std::string &fileExtension)
{
    fs::path targetPath = basePath / subFolder;

    if (fs::exists(targetPath) && fs::is_directory(targetPath))
    {
        for (const auto &entry : fs::directory_iterator(targetPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == fileExtension)
            {
                std::ofstream ofs(entry.path(), std::ios::trunc); // Open in truncate mode to clear the content
                if (ofs)
                {
                    std::cout << "Cleared content of: " << entry.path() << std::endl;
                }
                else
                {
                    std::cerr << "Failed to clear content of: " << entry.path() << std::endl;
                }
            }
        }
    }
    else
    {
        std::cerr << "Directory does not exist or is not accessible: " << targetPath << std::endl;
    }
}

// Function to process multiple folders and clear specified files
void processFolders(const std::vector<std::string> &folderPaths)
{
    for (const auto &folderPath : folderPaths)
    {
        fs::path basePath(folderPath);

        if (fs::exists(basePath) && fs::is_directory(basePath))
        {
            std::cout << "Processing folder: " << basePath << std::endl;

            // Clear .txt files in the logs subfolder
            clearFilesInFolder(basePath, "logs", ".txt");

            // Clear .bin files in the snapshots subfolder
            clearFilesInFolder(basePath, "snapshots", ".bin");
        }
        else
        {
            std::cerr << "Invalid folder path: " << folderPath << std::endl;
        }
    }
}

std::vector<std::string> readServerConfig(const std::string &configFile)
{
    std::ifstream file(configFile);
    std::vector<std::string> servers;

    if (!file.is_open()) // Check if the file opened successfully
    {
        std::cerr << "Error: Could not open the config file: " << configFile << std::endl;
        return servers; // Return an empty vector
    }

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Reading configuration file..." << std::endl;

    std::string line;
    while (std::getline(file, line))
    {
        servers.push_back(line); // Add each line to the vector
    }

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Finished reading configuration file." << std::endl;

    return servers;
}
// void *shutdown_monitor(void *arg)
// {
//     // Cast the argument back to the correct type
//     auto *server_data = static_cast<std::pair<std::shared_ptr<std::atomic<bool>>, grpc::Server *> *>(arg);
//     auto shutdown_flag = server_data->first; // Shared pointer to the shutdown flag
//     auto *grpc_server = server_data->second; // Pointer to the gRPC server

//     // Monitor the shutdown flag
//     while (!shutdown_flag->load())
//     {
//         sleep(1); // Avoid busy-waiting
//     }

//     // Shutdown signal detected
//     std::cout << "Shutdown signal detected. Stopping server..." << std::endl;
//     grpc_server->Shutdown();

//     return nullptr;
// }

// Function to start a gRPC server for each address:port
void startServer(const string &address, int serverIndex)
{

    grpc::ServerBuilder builder;
    auto shutdown_flag = std::make_shared<std::atomic<bool>>(false);
    auto bigTable = std::make_shared<BigTable>();
    auto bigTable2 = std::make_shared<BigTable>();
    auto bigTable3 = std::make_shared<BigTable>();
    // bigTable->put("austin#account", "password", "123");
    // bigTable->put("austin#account", "name", "austin");
    // bigTable->put("mahika#account", "password", "123");
    // bigTable->put("mahika#account", "name", "mahika");
    // bigTable->put("mike#account", "password", "123");
    // bigTable->put("mike#account", "name", "mike");
    // bigTable->put("shriya#account", "password", "123");
    // bigTable->put("shriya#account", "name", "shriya");
    // bigTable->put("linh#account", "password", "123");
    // bigTable->put("linh#account", "name", "linh");

    /// TODO:
    // Server Manager needs to know from coordinator who is the primary server and the list of servers in its cluster

    auto serverManager = std::make_shared<ServerManager>(address);
    // serverManager->set_primary_server("127.0.0.1:50051");
    // serverManager->add_server("127.0.0.1:50051");
    // serverManager->add_server("127.0.0.1:50052");
    // serverManager->add_server("127.0.0.1:50053");
    serverManager->setCoordinatorServer(coordinatorAddress);

    auto tabletManager = std::make_shared<TabletManager>();
    // tabletManager->calculateTabletBoundaries('a', 'z', 3);

    // initialize 2 vectors of strings for snapshot paths and log paths
    // vector<filesystem::path> snapshotPaths;
    // vector<filesystem::path> logPaths;
    // for (int i = 0; i < 3; i++)
    // {
    //     filesystem::path snapshotPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "snapshots" / (to_string(i) + ".bin");
    //     filesystem::path logPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".txt");
    // }

    auto sharedContext = std::make_shared<SharedContext>(serverManager, tabletManager);
    // tabletManager->set_current_tablet_index(0);
    tabletManager->set_tablet_version(0, 0);
    tabletManager->set_tablet_version(1, 0);
    tabletManager->set_tablet_version(2, 0);
    tabletManager->set_tablet(0, bigTable);
    tabletManager->set_tablet(1, bigTable2);
    tabletManager->set_tablet(2, bigTable3);
    sharedContext->getTabletManager()->set_current_tablet(bigTable);
    std::cout << "SharedContext address during initialization: " << sharedContext.get() << std::endl;

    CoordinatorLogic coordinatorLogic(sharedContext);

    ServiceLogic service(sharedContext, shutdown_flag); // Your service implementation

    InterServerLogic interServerLogic(sharedContext);

    ServerMappingArgs serverMappingArgs;
    ServerMappingReply serverMappingReply;

    grpc::ClientContext clientContext;
    serverMappingArgs.set_ipaddressport(address);
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);
    builder.RegisterService(&interServerLogic);
    builder.RegisterService(&coordinatorLogic);

    unique_ptr<grpc::Server> server(builder.BuildAndStart());
    logWithTimestamp(" ") << "Requesting server mapping from coordinator" << sharedContext->getServerManager()->get_coordinator_address() << endl;
    // std::this_thread::sleep_for(std::chrono::seconds(60));
    grpc::Status status = sharedContext->getServerManager()->get_coordinator_stub()->requestServerMapping(&clientContext, serverMappingArgs, &serverMappingReply);
    if (!status.ok())
    {
        logWithTimestamp(address) << "Failed to get server mapping from coordinator" << endl;
    }
    else
    {
        logWithTimestamp(address) << "Server mapping received from coordinator" << endl;
        string startChar = serverMappingReply.startchar();
        string endChar = serverMappingReply.endchar();
        string primaryServer = serverMappingReply.primaryserver();
        logWithTimestamp(address) << "Primary server is " << primaryServer << endl;
        logWithTimestamp(address) << "Start char is " << startChar[0] << endl;
        logWithTimestamp(address) << "End char is " << endChar[0] << endl;
        sharedContext->getTabletManager()->calculateTabletBoundaries(startChar[0], endChar[0], 3);
        sharedContext->getServerManager()->set_primary_server(primaryServer);
        sharedContext->getServerManager()->set_status(true);
    }
    // pthread_t shutdown_thread;
    // auto *server_data = new std::pair<std::shared_ptr<std::atomic<bool>>, grpc::Server *>(shutdown_flag, server.get());
    // if (pthread_create(&shutdown_thread, nullptr, shutdown_monitor, server_data) != 0)
    // {
    //     std::cerr << "Failed to create shutdown monitor thread." << std::endl;
    //     delete server_data;
    //     return;
    // }

    logWithTimestamp(address) << "Server " << serverIndex << " listening on " << address << endl;

    // Checkpoint recovery logic

    if (address == sharedContext->getServerManager()->get_primary_server())
    {
        logWithTimestamp(address) << "No need to recover, I am the primary server" << endl
                                  << flush;
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            CheckPointReply checkPointReply = fetchCheckPoint(sharedContext, i);

            // check whether checkpoint is null, then check whether the version returned is same as the version in the tablet manager
            // if (checkPointReply == nullptr)
            // {
            //     logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Checkpoint is null" << endl;
            //     continue;
            // }
            shared_ptr<BigTable> tempBigTable = sharedContext->getTabletManager()->get_tablet(i);

            filesystem::path currentPath = filesystem::current_path();
            string address = sharedContext->getServerManager()->get_address();
            string sanitizedAddress = sanitizeAddress(address);
            filesystem::path currentSnapshotPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "snapshots" / (to_string(i) + ".bin");
            filesystem::path currentLogPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".txt");
            int32_t checkPointVersion = getCheckpointVersion(currentSnapshotPath.string());
            logWithTimestamp(address) << "Checkpoint version after fetching is " << checkPointVersion << endl;
            logWithTimestamp(address) << "Checkpoint version in reply is " << checkPointReply.checkpointversion() << endl;
            // filesystem::path snapshotFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "snapshots" / (to_string(i) + ".bin");
            // filesystem::path logFilePath = currentPath / serverRecoveryPath / (sanitizedAddress) / "logs" / (to_string(i) + ".txt");

            if (checkPointReply.checkpointversion() == checkPointVersion)
            {
                // recover logs from the log file and update the big table
                // overwrite the current log file with the new log file
                // filesystem::path currentMetaFilePath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".meta");
                // size_t lineCount = getCurrentLineCount(currentMetaFilePath.string());
                fetchMissingLogsByPosition(currentLogPath.string(), sharedContext, i);

                // logWithTimestamp(address) << "Log file path in recovery is" << checkPointReply.logfilepath() << endl;
                // filesystem::path currentSnapshotPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "snapshots" / (to_string(i) + ".bin");

                // filesystem::path currentLogPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".txt");
                // overwriteFile(checkPointReply.logfilepath(), currentLogPath);

                // loadTableToBigTable(currentSnapshotPath, tempBigTable);
                // // processLogFile(currentLogPath, tempBigTable);
                // readLogRecords(currentLogPath, tempBigTable);
                // serializeBigTable(bigTable, currentSnapshotPath);
                // update the big table with the logs
            }
            else
            {
                logWithTimestamp(address) << "Checkpoint version is not same as the version in the tablet manager" << endl;
                // recover from the snapshot file and log file and update the big table
                // filesystem::path currentSnapshotPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "snapshots" / (to_string(i) + ".bin");
                // filesystem::path currentLogPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(serverManager->get_address())) / "logs" / (to_string(i) + ".txt");
                // overwriteFile(checkPointReply.logfilepath(), currentLogPath);
                // overwriteFile(checkPointReply.snapshotfilepath(), currentSnapshotPath);
                fetchCheckpointFile(currentSnapshotPath.string(), sharedContext, i);
                fetchLogFile(currentLogPath.string(), sharedContext, i);
                // fetchMissingLogsByPosition(currentLogPath.string(), sharedContext, i);
                // loadTableToBigTable(currentSnapshotPath, tempBigTable);
                // // processLogFile(currentLogPath, tempBigTable);
                // readLogRecords(currentLogPath, tempBigTable);
                // serializeBigTable(bigTable, currentSnapshotPath);
                sharedContext->getTabletManager()->set_tablet_version(i, checkPointReply.checkpointversion());
                // sharedContext->getTabletManager()->set_current_tablet_index(i);
            }
        }
    }

    TabletIndexArgs tabletIndexArgs;
    TabletIndexReply tabletIndexReply;

    if (address == sharedContext->getServerManager()->get_primary_server())
    {
        grpc::ClientContext clientContextForCoordinator;
        AllAddressReply allAddressReply;
        vector<string> allAddresses;
        AllAddressArgs allAddressArgs;
        allAddressArgs.set_ipaddressport(address);
        grpc::Status status = sharedContext->getServerManager()->get_coordinator_stub()->fetchAllAddress(&clientContextForCoordinator, allAddressArgs, &allAddressReply);

        if (!status.ok())
        {
            logWithTimestamp(address) << "Failed to fetch all addresses from coordinator" << endl;
        }
        else
        {
            allAddresses.assign(allAddressReply.addresses().begin(), allAddressReply.addresses().end());
            for (auto server : allAddresses)
            {
                logWithTimestamp(address) << "Adding server " << server << " to server manager" << endl;
                sharedContext->getServerManager()->add_server(server);
            }
            logWithTimestamp(address) << "All addresses received from coordinator" << endl;
        }
    }

    // pthread_mutex_t *currentTabletIndexMutex = sharedContext->getCurrentTabletIndexMutex();
    // pthread_mutex_lock(currentTabletIndexMutex);
    grpc::ClientContext contextForFetchTablet;

    grpc::Status fetchTabletStatus = sharedContext->getServerManager()->get_primary_stub()->fetchCurrentTabletIndex(&contextForFetchTablet, tabletIndexArgs, &tabletIndexReply);

    // if (address == sharedContext->getServerManager()->get_primary_server())
    // {
    //     sharedContext->getTabletManager()->set_current_tablet_index(0);
    //     sharedContext->getTabletManager()->set_current_tablet(bigTable);
    //     sharedContext->getTabletManager()->set_tablet(0, bigTable);
    // }

    if (!fetchTabletStatus.ok())
    {
        logWithTimestamp(address) << "Failed to fetch current tablet index from primary server" << endl;
    }
    else
    {
        logWithTimestamp(address) << "Current tablet index is " << tabletIndexReply.tabletindex() << endl;
        sharedContext->getTabletManager()->set_current_tablet_index(tabletIndexReply.tabletindex());
        grpc::ClientContext tempClientContext;
        LoadTabletArgs loadTabletArgs;
        LoadTabletReply loadTabletReply;
        loadTabletArgs.set_tabletno(tabletIndexReply.tabletindex());
        // create stub for current server and then call loadNewTablet
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        auto stub = InterServer::NewStub(channel);
        if (loadTabletArgs.tabletno() != -1)
        {
            grpc::Status loadStatus = stub->loadNewTablet(&tempClientContext, loadTabletArgs, &loadTabletReply);
            if (!loadStatus.ok())
            {
                ABSL_LOG(ERROR) << "Failed to load tablet: " << endl;
            }
        }
        // grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->loadNewTablet(&tempClientContext, loadTabletArgs, &loadTabletReply);
    }
    logWithTimestamp(address) << "Current tablet index in main is " << sharedContext->getTabletManager()->get_current_tablet_index() << endl
                              << flush;
    server->Wait(); // This will block until the server is shut down
    // pthread_join(shutdown_thread, nullptr);
    // delete server_data;
}

int main(int argc, char *argv[])
{

    if (argc == 2 && string(argv[1]) == "clear")
    {
        std::vector<std::string> folderPaths = {
            "../../server/src/checkpoints/127.0.0.1_50051",
            "../../server/src/checkpoints/127.0.0.1_50052",
            "../../server/src/checkpoints/127.0.0.1_50053",
            "../../server/src/checkpoints/127.0.0.1_50054",
            "../../server/src/checkpoints/127.0.0.1_50055",
            "../../server/src/checkpoints/127.0.0.1_50056",
            "../../server/src/checkpoints/127.0.0.1_50057",
            "../../server/src/checkpoints/127.0.0.1_50058",
            "../../server/src/checkpoints/127.0.0.1_50059"};

        // Simulate reading from a list of folder paths provided in the argument (comma-separated)
        // std::vector<std::string> folderPaths;
        // size_t start = 0, end;
        // while ((end = folderListArg.find(',', start)) != std::string::npos)
        // {
        //     folderPaths.push_back(folderListArg.substr(start, end - start));
        //     start = end + 1;
        // }
        // folderPaths.push_back(folderListArg.substr(start));

        // Process the folders
        processFolders(folderPaths);
    }

    if (argc == 2 && string(argv[1]) == "compare")
    {
        compareFiles("../../server/src/checkpoints/127.0.0.1_50051/logs/1.txt", "../../server/src/checkpoints/127.0.0.1_50052/logs/1.txt");
        return 1;
    }

    // Check if the config file path and server index are provided
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <config_file> <server_index>" << endl;
        return 1;
    }

    string configFile = argv[1];
    int serverIndex = atoi(argv[2]);

    // Read server addresses and ports from the configuration file
    vector<string> serverAddresses = readServerConfig(configFile);

    // Validate the server index
    if (serverIndex < 1 || serverIndex > serverAddresses.size())
    {
        cerr << "Invalid server index. Please specify an index between 1 and " << serverAddresses.size() << endl;
        return 1;
    }

    // Start the selected server
    string selectedAddress = serverAddresses[serverIndex - 1]; // Adjusting for 0-based index
    startServer(selectedAddress, serverIndex);

    return 0;
}
